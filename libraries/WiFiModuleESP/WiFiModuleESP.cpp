#include "Arduino.h"
#include "WiFiModuleESP.h"



uint16_t WiFiModuleESP::DownloadRequest::nextFreeKey = 0;



WiFiModuleESP::WiFiModuleESP(SdFat& sdFat) :
	sdFat(sdFat)
{
	pServer = NULL;
	SetState(State::wait);
}

void WiFiModuleESP::Update()
{
	SwitchState();

	switch (currentState)
	{
		case State::wait:
			WaitProcedure();
			break;

		case State::start:
			StartProcedure();
			break;

		case State::offline:
			OfflineProcedure();
			break;

		case State::online:
			OnlineProcedure();
			break;
	}
}

bool WiFiModuleESP::TakeMessage(int message, void* sender, void** additionalData)
{
	switch (message)
	{
		case MESSAGE_TIME_KEEPER:
		{
			switch (*((uint16_t*)additionalData))
			{
				case MESSAGE_START_COLLECTING_LOGS:
					if(TryAcquireSemaphore(0))
						serialListener.StartCollectingLogs();
					break;

				case MESSAGE_STOP_COLLECTING_LOGS:
					serialListener.StopCollectingLogs();
					ReleaseSemaphore(0);
					break;

				case MESSAGE_WRITE_CONFIG_FILE:
					if (TryAcquireSemaphore(1))
					{
						WriteConfigFile();
						ReleaseSemaphore(1);
					}
					break;

				case MESSAGE_RESTART:
					ESP.restart();
					break;

				case MESSAGE_FORMAT:
					if (TryAcquireSemaphore(7))
					{
						File file;
						file.open("/FormatFlag.txt", O_WRITE | O_CREAT | O_TRUNC);
						file.write('a');
						file.close();
						ReleaseSemaphore(7);
					}
					break;

				case MESSAGE_WEAK_FORMAT:
					if (TryAcquireSemaphore(7))
					{
						File file;
						file.open("/WeakFormatFlag.txt", O_WRITE | O_CREAT | O_TRUNC);
						file.write('a');
						file.close();
						ReleaseSemaphore(7);
					}
					break;

				case MESSAGE_CHECK_WIFI_CONNECTION:													//
				{																					//
					if (WiFi.status() == WL_CONNECTED)												//
					{																				//
						uint16_t* subMessage = new uint16_t();										//
						*subMessage = MESSAGE_CHECK_WIFI_CONNECTION;								//
						timeKeeper.RequestMessage(millis() + 5000, this, (void**)subMessage);		//
					}																				//
					else																			//
					{																				//
						#if DEBUG_WIFIMODULEESP														//
						String log;																	// Co kilka sekund sprawdzamy polaczenie z AP,
						log += F("2_41");															// aby moc zareagowac na ewentualne rozlaczenie.
						DEBUG_WIFIMODULEESP_LOG(log);												// W razie jego wystapienia restartujemy mikrokontroler
						#endif																		//
																									//
						CloseMassMemory();															//
						if (TryAcquireSemaphore(8))													//
						{																			//
							uint16_t* subMessage = new uint16_t();									//
							*subMessage = MESSAGE_RESTART;											//
							timeKeeper.RequestMessage(millis() + 100, this, (void**)subMessage);	//
						}																			//
					}																				//
																									//
					break;																			//
				}																					//
			}

			delete ((uint16_t*)additionalData);
			return true;
		}

		case MESSAGE_CONNECT_TASK_END:
		{
			bool error = false;

			if (*((uint8_t*)additionalData) == 2)
			{
				error = true;

				uint8_t connectRetries = config.connectRetries.toInt();
				uint8_t connectRetriesCurrent = config.connectRetriesCurrent.toInt();
				++connectRetriesCurrent;

				if (connectRetriesCurrent > connectRetries)		//
				{												//
					config.ssid = "";							//
					config.password = "";						//
					config.connectRetriesCurrent = "0";			//
																// Jesli przekroczylismy juz liczbe prob polaczenia,
					#if DEBUG_WIFIMODULEESP						// to usuwamy dane dostepowe
					String log;									//
					log += F("2_51\x1F");						//
					log += connectRetries;						//
					DEBUG_WIFIMODULEESP_LOG(log);				//
					#endif										//
				}												//
				else																//
				{																	//
					config.connectRetriesCurrent = String(connectRetriesCurrent);	//
																					//
					#if DEBUG_WIFIMODULEESP											//
					String log;														// Odnotowujemy kolejna nieudana probe polaczenia
					log += F("2_52\x1F");											//
					log += connectRetriesCurrent;									//
					DEBUG_WIFIMODULEESP_LOG(log);									//
					#endif															//
				}																	//
			}
			else
			{
				if (*((uint8_t*)additionalData) == 1)
				{
					uint16_t* subMessage = new uint16_t();									//
					*subMessage = MESSAGE_CHECK_WIFI_CONNECTION;							// W razie podlaczenia do zewnetrznego AP, rozpoczynamy monitorowanie tego polaczenia
					timeKeeper.RequestMessage(millis() + 5000, this, (void**)subMessage);	//
					SetState(State::online);
				}
				else
				{
					SetState(State::offline);
				}

				config.connectRetriesCurrent = "0";
			}

			if (TryAcquireSemaphore(1))		//
			{								//
				WriteConfigFile();			// Aktualizujemy dane w pamieci masowej
				ReleaseSemaphore(1);		//
			}								//

			if (error)
			{
				CloseMassMemory();
				if (TryAcquireSemaphore(8))													//
				{																			//
					uint16_t* subMessage = new uint16_t();									// Przygotowujemy sie do
					*subMessage = MESSAGE_RESTART;											// restartu
					timeKeeper.RequestMessage(millis() + 100, this, (void**)subMessage);	//
				}																			//
			}

			delete ((uint8_t*)additionalData);
			return true;
		}

		case MESSAGE_NETWORK_SCAN_TASK_END:
		{
			asyncRequest.locked = false;
			uint8_t networkCount = WiFi.scanComplete();
			for (uint8_t i = 0; i < networkCount; ++i)
			{
				asyncRequest.result += WiFi.SSID(i);
				asyncRequest.result += ' ';
				asyncRequest.result += WiFi.RSSI(i);
				asyncRequest.result += ' ';
				asyncRequest.result += WiFi.channel(i);
				asyncRequest.result += ' ';
				asyncRequest.result += WiFi.isHidden(i);
				asyncRequest.result += '\n';
			}
			WiFi.scanDelete();

			return true;
		}

		case MESSAGE_SERIAL_HAVE_DATA:
		{
			byte* receivedData = (byte*)additionalData;		// Dla wygody odrazu rzutuje sobie ten wskaznik
			short subMessage = ((short*)receivedData)[0];			// 2 pierwsze bajty 'additionalData' zawieraja pod-wiadomosc

			switch (subMessage)
			{
				case MESSAGE_GET_RESPONSE:
				{
					float currentTemperature = ((float*)receivedData)[1];
					float maxTemperature = ((float*)receivedData)[2];
					float minTemperature = ((float*)receivedData)[3];
					short delay = ((short*)receivedData)[8];
					short sensitivity = ((short*)receivedData)[9];
					short overtake = ((short*)receivedData)[10];
					short nightThreshold = ((short*)receivedData)[11];
					short light = ((short*)receivedData)[12];
					short drag = ((short*)receivedData)[13];
					short overtakeVertical = ((short*)receivedData)[14];
					short overtakeHorizontal = ((short*)receivedData)[15];

					String solarControllerState;
					switch (receivedData[2])
					{
						case 0:
							solarControllerState = "idle";
							break;

						case 1:
							solarControllerState = "sunAware";
							break;

						case 2:
							solarControllerState = "nightAware";
							break;

						case 3:
							solarControllerState = "followSun";
							break;

						case 4:
							solarControllerState = "manual";
							break;

						case 5:
							solarControllerState = "foldDown";
							break;

						case 6:
							solarControllerState = "sleep";
							break;
					}

					String wifiModuleESPState;
					switch (currentState)
					{
						case State::start:
							wifiModuleESPState = "start";
							break;

						case State::offline:
							wifiModuleESPState = "offline";
							break;

						case State::online:
							wifiModuleESPState = "online";
							break;
					}

					asyncRequest.result = solarControllerState;
					asyncRequest.result += F('\n');
					asyncRequest.result += wifiModuleESPState;
					asyncRequest.result += F('\n');
					asyncRequest.result += delay;
					asyncRequest.result += F('\n');
					asyncRequest.result += sensitivity;
					asyncRequest.result += F('\n');
					asyncRequest.result += overtake;
					asyncRequest.result += F('\n');
					asyncRequest.result += nightThreshold;
					asyncRequest.result += F('\n');
					asyncRequest.result += bitRead(receivedData[3], 0);
					asyncRequest.result += F(' ');
					asyncRequest.result += bitRead(receivedData[3], 1);
					asyncRequest.result += F('\n');
					asyncRequest.result += light;
					asyncRequest.result += F('\n');
					asyncRequest.result += drag;
					asyncRequest.result += F('\n');
					asyncRequest.result += overtakeVertical;
					asyncRequest.result += F('\n');
					asyncRequest.result += overtakeHorizontal;
					asyncRequest.result += F('\n');
					asyncRequest.result += currentTemperature;
					asyncRequest.result += F('\n');
					asyncRequest.result += maxTemperature;
					asyncRequest.result += F('\n');
					asyncRequest.result += minTemperature;

					asyncRequest.locked = false;
					break;
				}

				case MESSAGE_SET_RESPONSE:
				{
					asyncRequest.result = receivedData[2];
					asyncRequest.locked = false;
					break;
				}

				case MESSAGE_MOTOR_RESPONSE:
				{
					asyncRequest.result = receivedData[2];
					asyncRequest.locked = false;
					break;
				}
			}

			return true;
		}
	}

	return false;
}

void WiFiModuleESP::SetState(State newState, uint32_t initializationData)
{
	nextState = newState;
	nextStateInit = bit(sizeof(initializationData) * 8 - 1) | initializationData;
	updateManager.Attach(this);

	#if DEBUG_WIFIMODULEESP
	String log;

	switch (nextState)
	{
		case State::wait:
			log += F("2_101");
			break;

		case State::start:
			log += F("2_102");
			break;

		case State::offline:
			log += F("2_103");
			break;

		case State::online:
			log += F("2_104");
			break;
	}

	DEBUG_WIFIMODULEESP_LOG(log);
	#endif
}

void WiFiModuleESP::SwitchState()
{
	if (nextStateInit)
	{
		currentState = nextState;
		updateInfo.data_1 = nextStateInit;

		nextStateInit = 0;
	}
}

void WiFiModuleESP::WaitProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		updateInfo.data_1 = 0;
		updateInfo.time_1 = millis() + 5000;

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	switch (updateInfo.data_1)
	{
		case 0:
		{
			if ((long)(millis() - updateInfo.time_1) >= 0)		//
			{													//
				if (sdFat.begin(16, SPI_HALF_SPEED))			//
				{												//
					#if DEBUG_WIFIMODULEESP						//
					String log;									//
					log += F("2_131");							//
					DEBUG_WIFIMODULEESP_LOG(log);				//
					#endif										// "Uruchomienie" pamieci masowej
																//
					bitSet(updateInfo.data_1, 0);				//
					updateInfo.time_1 = millis() + 300;			//
				}												//
				else											//
				{												//
					ESP.restart();								//
				}												//
			}													//

			break;
		}

		case 1:
		{
			if ((long)(millis() - updateInfo.time_1) >= 0)			//
			{														//
				if (sdFat.exists("/FormatFlag.txt"))				//
				{													//
					sdFat.wipe();									//
																	//
					if (!sdFat.begin(16, SPI_HALF_SPEED))			//
						ESP.restart();								// Format, albo oczyszczanie pamieci
				}													//
				else												//
				{													//
					if (sdFat.exists("/WeakFormatFlag.txt"))		//
						WeakFormat();								//
				}													//
																	//
				bitSet(updateInfo.data_1, 1);						//
				updateInfo.time_1 = millis() + 300;					//
			}														//

			break;
		}

		case 3:
		{
			if ((long)(millis() - updateInfo.time_1) >= 0)			//
			{														//
				#if DEBUG_WIFIMODULEESP								//
				String log;											//
				log += F("2_132");									//
				DEBUG_WIFIMODULEESP_LOG(log);						//
				#endif												// Inicjalizacja pamieci masowej dla
																	// obiektu serialListener
				serialListener.AddLogType("b");						//
				serialListener.PrepareMassMemory();					//
				if (TryAcquireSemaphore(0))							//
					serialListener.StartCollectingLogs();			//
																	//
				bitSet(updateInfo.data_1, 2);						//
				updateInfo.time_1 = millis() + 300;					//
			}														//

			break;
		}

		case 7:
		{
			if ((long)(millis() - updateInfo.time_1) >= 0)			// Przejscie do stanu start
				SetState(State::start);								//

			break;
		}
	}
}

void WiFiModuleESP::StartProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		serialListener.Attach(SERIAL_WIFIMODULEESP, this);
		PrepareMassMemory();

		if(!sdFat.exists("/Config/WiFiModuleESP.txt"))								//
		{																			//
			#if DEBUG_WIFIMODULEESP													//
			String log;																//
			log += F("2_201");														//
			DEBUG_WIFIMODULEESP_LOG(log);											//
			#endif																	//
																					//
			config.ssid = "";														//
			config.password = "";													//
			config.connectRetries = "3";											//
			config.connectRetriesCurrent = "0";										//
			config.apChannel = "1";													// Szukamy w pamieci masowej pliku konfiguracyjnego, dedykowanego dla tej klasy,
			WriteConfigFile();														// a nastepnie "synchronizujemy" jego zawartosc z polem 'config'
		}																			//
		else																		//
		{																			//
			#if DEBUG_WIFIMODULEESP													//
			String log;																//
			log += F("2_202");														//
			DEBUG_WIFIMODULEESP_LOG(log);											//
			#endif																	//
																					//
			ReadConfigFile();														//
		}																			//

		if(sdFat.exists("/Assets/AssetFileNames.txt"))													//
		{																								//
			#if DEBUG_WIFIMODULEESP																		//
			String log;																					//
			log += F("2_203");																			//
			DEBUG_WIFIMODULEESP_LOG(log);																//
			#endif																						//
																										//
			char line[128];																				//
			File file;																					//
			file.open("/Assets/AssetFileNames.txt", O_READ);											//
																										//
			while (file.available())																	//
			{																							//
				ReadLineFromFile(file, line, 128);														//
				String lineString = String(line);														//
				int16_t spaceIndex = lineString.indexOf(' ');											//
																										//
				if (spaceIndex != -1 && spaceIndex > 0 && spaceIndex < lineString.length() - 1)			//
				{																						//
					AssetFileName* pAssetFileName = new AssetFileName();								//
					pAssetFileName->shortName = "/Assets/";												//
					pAssetFileName->shortName += lineString.substring(0, spaceIndex);					//
					pAssetFileName->realName = "/Assets/";												//
					pAssetFileName->realName += lineString.substring(spaceIndex + 1);					//
																										//
					if (sdFat.exists(pAssetFileName->shortName.c_str()))								//
					{																					//
						assetFileNames.add(pAssetFileName);												// Szukamy w pamieci masowej pliku mapujacego zbyt dlugie nazwy zasobow (SPIFFS nie dopuszcza nazw powyzej 31 znakow)
																										// na ich krotsze odpowiedniki
						#if DEBUG_WIFIMODULEESP															//
						String log;																		//
						log += F("2_204\x1F");															//
						log += pAssetFileName->shortName;												//
						log += '\x1F';																	//
						log += pAssetFileName->realName;												//
						DEBUG_WIFIMODULEESP_LOG(log);													//
						#endif																			//
					}																					//
					else																				//
					{																					//
						delete pAssetFileName;															//
																										//
						#if DEBUG_WIFIMODULEESP															//
						String log;																		//
						log += F("2_205\x1F");															//
						log += pAssetFileName->shortName;												//
						DEBUG_WIFIMODULEESP_LOG(log);													//
						#endif																			//
					}																					//
				}																						//
				else																					//
				{																						//
					#if DEBUG_WIFIMODULEESP																//
					String log;																			//
					log += F("2_206");																	//
					DEBUG_WIFIMODULEESP_LOG(log);														//
					#endif																				//
				}																						//
			}																							//
																										//
			file.close();																				//
		}																								//

		new ConnectTask(config.ssid, config.password, config.apChannel, this);			// Przechodzimy w tryb "uspienia" i czekamy na zakonczenie
		updateManager.Detach(this);														// pracy ConnectTask
		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}
}

void WiFiModuleESP::OfflineProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		CreateServer();

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if (uploadRequest.active)										//
	{																//
		if ((long)(millis() - uploadRequest.timeOut) >= 0)			//
		{															//
			if (TryAcquireSemaphore(6))								//
			{														//
				uploadRequest.EndUpload();							// Monitorujemy pliki przesylane od klienta na serwer.
				#if DEBUG_WIFIMODULEESP								// W razie zbyt dlugiego transferu, przerywamy operacje
				String log;											//
				log += F("2_221");									//
				DEBUG_WIFIMODULEESP_LOG(log);						//
				#endif												//
				ReleaseSemaphore(6);								//
			}														//
		}															//
	}																//

	if ((long)(millis() - updateInfo.time_1) >= 0)					//
	{																//
		String log;													//
																	//
		switch (updateInfo.data_1)									//
		{															//
			case 0:													//
				log += F("b2_222\x1F");								//
				log += ESP.getFreeHeap();							//
				break;												//
																	//
			case 1:													// Generujemy logi o charakterze ciaglym:
				log += F("b2_223\x1F");								// -Ilosc wolnej pamieci RAM
				log += updateManager.GetMaxLoopDuration();			// -Predkosc petli glownej
				break;												//
		}															//
																	//
		serialListener.Log(log);									//
																	//
		++updateInfo.data_1;										//
		if (updateInfo.data_1 > 1)									//
			updateInfo.data_1 = 0;									//
																	//
		updateInfo.time_1 = millis() + 1000;						//
	}																//
}

void WiFiModuleESP::OnlineProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		CreateServer();

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if (uploadRequest.active)									//
	{															//
		if ((long)(millis() - uploadRequest.timeOut) >= 0)		//
		{														//
			if (TryAcquireSemaphore(6))							//
			{													//
				uploadRequest.EndUpload();						// Monitorujemy pliki przesylane od klienta na serwer.
				#if DEBUG_WIFIMODULEESP							// W razie zbyt dlugiego transferu, przerywamy operacje
				String log;										//
				log += F("2_221");								//
				DEBUG_WIFIMODULEESP_LOG(log);					//
				#endif											//
				ReleaseSemaphore(6);							//
			}													//
		}														//
	}															//

	if ((long)(millis() - updateInfo.time_1) >= 0)				//
	{															//
		String log;												//
																//
		switch (updateInfo.data_1)								//
		{														//
			case 0:												//
				log += F("b2_222\x1F");							//
				log += ESP.getFreeHeap();						//
				break;											//
																// Generujemy logi o charakterze ciaglym:
			case 1:												// -Ilosc wolnej pamieci RAM
				log += F("b2_223\x1F");							// -Predkosc petli glownej
				log += updateManager.GetMaxLoopDuration();		//
				break;											//
		}														//
																//
		serialListener.Log(log);								//
																//
		++updateInfo.data_1;									//
		if (updateInfo.data_1 > 1)								//
			updateInfo.data_1 = 0;								//
																//
		updateInfo.time_1 = millis() + 1000;					//
	}															//
}

void WiFiModuleESP::CreateServer()
{
	if (pServer)
	{
		delete pServer;
		pServer = NULL;
	}

	pServer = new AsyncWebServer(80);

	pServer->onNotFound(std::bind(&WiFiModuleESP::CallbackNotFound, this, std::placeholders::_1));
	pServer->on("/format", HTTP_ANY, std::bind(&WiFiModuleESP::CallbackFormat, this, std::placeholders::_1));
	pServer->on("/weak-format", HTTP_ANY, std::bind(&WiFiModuleESP::CallbackWeakFormat, this, std::placeholders::_1));
	pServer->on("/shutdown", HTTP_ANY, std::bind(&WiFiModuleESP::CallbackShutdown, this, std::placeholders::_1));
	pServer->on("/restart", HTTP_ANY, std::bind(&WiFiModuleESP::CallbackRestart, this, std::placeholders::_1));
	pServer->on("/upload", HTTP_ANY, std::bind(&WiFiModuleESP::CallbackUpload, this, std::placeholders::_1), NULL, std::bind(&WiFiModuleESP::CallbackUploadBodyHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
	pServer->on("/set-time", HTTP_ANY, std::bind(&WiFiModuleESP::CallbackSetTime, this, std::placeholders::_1));
	pServer->on("/set-network-credentials", HTTP_ANY, std::bind(&WiFiModuleESP::CallbackSetNetworkCredentials, this, std::placeholders::_1));
	pServer->on("/set-ap-channel", HTTP_ANY, std::bind(&WiFiModuleESP::CallbackSetApChannel, this, std::placeholders::_1));
	pServer->on("/network-scan", HTTP_ANY, std::bind(&WiFiModuleESP::CallbackNetworkScan, this, std::placeholders::_1));
	pServer->on("/start-collecting-logs", HTTP_ANY, std::bind(&WiFiModuleESP::CallbackStartCollectingLogs, this, std::placeholders::_1));
	pServer->on("/stop-collecting-logs", HTTP_ANY, std::bind(&WiFiModuleESP::CallbackStopCollectingLogs, this, std::placeholders::_1));
	pServer->on("/list-directory", HTTP_ANY, std::bind(&WiFiModuleESP::CallbackListDirectory, this, std::placeholders::_1));
	pServer->on("/switch-to-auto", HTTP_ANY, std::bind(&WiFiModuleESP::CallbackSwitchToAuto, this, std::placeholders::_1));
	pServer->on("/switch-to-manual", HTTP_ANY, std::bind(&WiFiModuleESP::CallbackSwitchToManual, this, std::placeholders::_1));
	pServer->on("/get-status", HTTP_ANY, std::bind(&WiFiModuleESP::CallbackGetStatus, this, std::placeholders::_1));
	pServer->on("/set-parameters", HTTP_ANY, std::bind(&WiFiModuleESP::CallbackSetParameters, this, std::placeholders::_1));
	pServer->on("/control-motor", HTTP_ANY, std::bind(&WiFiModuleESP::CallbackControlMotor, this, std::placeholders::_1));
	pServer->on("/async-request-result", HTTP_ANY, std::bind(&WiFiModuleESP::CallbackAsyncRequestResult, this, std::placeholders::_1));

	pServer->begin();

	#if DEBUG_WIFIMODULEESP				//
	String log;							//
	log += F("2_241");					// Utworzono serwer HTTP
	DEBUG_WIFIMODULEESP_LOG(log);		//
	#endif								//
}

void WiFiModuleESP::SuccessResponse(AsyncWebServerRequest* request, String* additionalString)
{
	String response = "Success";

	if (additionalString)
	{
		response += '\n';
		response += *additionalString;
	}

	request->send(200, "text/plain", response);
}

void WiFiModuleESP::ErrorResponse(AsyncWebServerRequest* request, String* additionalString)
{
	String response = "Error";

	if (additionalString)
	{
		response += '\n';
		response += *additionalString;
	}

	request->send(200, "text/plain", response);
}

bool WiFiModuleESP::BeginAsyncRequest(AsyncWebServerRequest* request)
{
	if (!asyncRequest.locked)
	{
		asyncRequest.locked = true;
		asyncRequest.invokeTime = millis();
		asyncRequest.url = request->url();
		return true;
	}

	return false;
}

String WiFiModuleESP::GetContentType(const String& path)
{
	char buff[sizeof(mimeTable[0].mimeType)];
	// Check all entries but last one for match, return if found
	for (size_t i = 0; i < sizeof(mimeTable) / sizeof(mimeTable[0]) - 1; i++) {
		strcpy_P(buff, mimeTable[i].endsWith);
		if (path.endsWith(buff)) {
			strcpy_P(buff, mimeTable[i].mimeType);
			return String(buff);
		}
	}
	// Fall-through and just return default type
	strcpy_P(buff, mimeTable[sizeof(mimeTable) / sizeof(mimeTable[0]) - 1].mimeType);
	return String(buff);
}

// WiFiModuleESP jest glownym kontrolerem w projekcie,
// wiec jego zadaniem jest okreslenie jakie pliki
// sa newralgiczne, a jekie mozemy uznac za smieci.
void WiFiModuleESP::WeakFormat()
{
	File root;
	root.open("/", O_READ);
	Ivanseidel::LinkedList<String> filesToKeep;
	filesToKeep.add("System Volume Information/");
	filesToKeep.add("jquery-3.3.1.min.js.gz");
	filesToKeep.add("UploadRequest.js.gz");
	char nameBuffer[256];
	WeakFormat(root, filesToKeep, nameBuffer, 256);
	root.close();
}

// Metoda rekurencyjna wykorzystywana
// przez metode powyzej
bool WiFiModuleESP::WeakFormat(File& folderFile, Ivanseidel::LinkedList<String>& filesToKeep, char* nameBuffer, uint16_t nameBufferLength)
{
	bool result = false;
	File file;
	Ivanseidel::LinkedList<String> filesToDelete;
	folderFile.rewind();

	while (file.openNext(&folderFile, O_READ))
	{
		file.getName(nameBuffer, nameBufferLength);
		String fileName = String(nameBuffer);

		if (file.isDir())
			fileName += '/';

		bool important = false;
		for (uint32_t i = 0; i < filesToKeep.size(); ++i)
		{
			if (fileName == filesToKeep.get(i))
			{
				important = true;
				break;
			}
		}

		if (important || file.isHidden() || file.isSystem() || file.isReadOnly())
		{
			result = true;
		}
		else
		{
			if (file.isDir() && WeakFormat(file, filesToKeep, nameBuffer, nameBufferLength))
				result = true;
			else
				filesToDelete.add(fileName);
		}

		file.close();
	}

	for (uint32_t i = 0; i < filesToDelete.size(); ++i)
	{
		if (filesToDelete.get(i).endsWith("/"))
		{
			file.open(&folderFile, filesToDelete.get(i).c_str(), O_READ);

			if (file.rmdir())
			{
				#if DEBUG_WIFIMODULEESP
				String log;
				log += F("2_301\x1F");
				log += filesToDelete.get(i);
				DEBUG_WIFIMODULEESP_LOG(log);
				#endif
			}
			else
			{
				#if DEBUG_WIFIMODULEESP
				String log;
				log += F("2_302\x1F");
				log += filesToDelete.get(i);
				DEBUG_WIFIMODULEESP_LOG(log);
				#endif
			}
		}
		else
		{
			file.open(&folderFile, filesToDelete.get(i).c_str(), O_WRITE);

			if (file.remove())
			{
				#if DEBUG_WIFIMODULEESP
				String log;
				log += F("2_303\x1F");
				log += filesToDelete.get(i);
				DEBUG_WIFIMODULEESP_LOG(log);
				#endif
			}
			else
			{
				#if DEBUG_WIFIMODULEESP
				String log;
				log += F("2_304\x1F");
				log += filesToDelete.get(i);
				DEBUG_WIFIMODULEESP_LOG(log);
				#endif
			}
		}

		file.close();
	}

	return result;
}

// Jej zadaniem jest przygotowanie wszystkich
// folderow potrzebnych do dzialania tej klasy
void WiFiModuleESP::PrepareMassMemory()
{
	File file;

	if (!file.open("/Config", O_READ))
		file.mkdir(File::cwd(), "/Config");
	file.close();

	if (!file.open("/Htdocs", O_READ))
		file.mkdir(File::cwd(), "/Htdocs");
	file.close();

	if (!file.open("/Assets", O_READ))
		file.mkdir(File::cwd(), "/Assets");
	file.close();
}

// WiFiModuleESP jest glownym kontrolerem w projekcie,
// wiec musi wiedziec jakie operacje w pamieci masowej
// moga miec miejsce (zarowno w nim, jak i w obiektach)
// narzedziowych). Ta metoda ma za zadanie zakonczenie
// tych operacji tak aby mozna bylo odmatowac pamiec,
// lub zrestartowac urzadzenie
void WiFiModuleESP::CloseMassMemory()
{
	serialListener.StopCollectingLogs();
	ReleaseSemaphore(0);
}

bool WiFiModuleESP::ReadLineFromFile(File& file, char* buffer, size_t bufferLength)
{
	char character;
	size_t i;

	for (i = 0; i < bufferLength; ++i)												//
	{																				//
		character = file.read();													//
																					//
		if (character == '\r' || character == '\n' || (int8_t)character == EOF)		//
		{																			//
			if (character == '\r' && file.peek() == '\n')							// Idzie po elementach tablicy 'buffer' przepisujac do nich zawartosc pliku
				file.read();														// az do napotkania konca lini, lub zapelnienia 'buffer'
																					//
			break;																	//
		}																			//
		else																		//
		{																			//
			buffer[i] = character;													//
		}																			//
	}																				//

	if (i < bufferLength)	//
	{						//
		buffer[i] = '\0';	// Cala linia zmiescila sie w 'buffer'
		return true;		//
	}						//
	else									//
	{										// Linia nie zmiescila sie w 'buffer',
		buffer[bufferLength - 1] = '\0';	// w tym momencie w 'buffer' znajduje sie ucieta linia
		return false;						//
	}										//
}

void WiFiModuleESP::ReadConfigFile()
{
	char line[128];
	File file;
	file.open("/Config/WiFiModuleESP.txt", O_READ);

	ReadLineFromFile(file, line, 128);
	config.ssid = String(line);

	ReadLineFromFile(file, line, 128);
	config.password = String(line);

	ReadLineFromFile(file, line, 128);
	config.connectRetries = String(line);

	ReadLineFromFile(file, line, 128);
	config.connectRetriesCurrent = String(line);

	ReadLineFromFile(file, line, 128);
	config.apChannel = String(line);

	file.close();
}

void WiFiModuleESP::WriteConfigFile()
{
	File file;
	file.open("/Config/WiFiModuleESP.txt", O_WRITE | O_CREAT | O_TRUNC);

	String line;
	line = config.ssid + '\n';
	file.write((uint8_t*)line.c_str(), line.length());

	line = "";
	line = config.password + '\n';
	file.write((uint8_t*)line.c_str(), line.length());

	line = "";
	line = config.connectRetries + '\n';
	file.write((uint8_t*)line.c_str(), line.length());

	line = "";
	line = config.connectRetriesCurrent + '\n';
	file.write((uint8_t*)line.c_str(), line.length());

	line = "";
	line = config.apChannel + '\n';
	file.write((uint8_t*)line.c_str(), line.length());

	file.close();
}

bool WiFiModuleESP::TryAcquireSemaphore(uint8_t operation)
{
	static const uint16_t avoidMask[] =
	{
		bit(8),
		bit(2) | bit(8),
		bit(1) | bit(8),
		bit(0) | bit(1) | bit(2) | bit(4) | bit(5) | bit(6) | bit(7) | bit(8),
		bit(3) | bit(6) | bit(8),
		bit(0) | bit(1) | bit(3) | bit(6) | bit(7) | bit(8),
		bit(8),
		bit(8),
		bit(0) | bit(1) | bit(2) | bit(6) | bit(7)
	};

	static const uint16_t blockMask[] =
	{
		~( bit(3) | bit(4) | bit(5) ),
		~( bit(3) | bit(5) ),
		~( bit(3) ),
		~0UL,
		~0UL,
		~0UL,
		~( bit(3) | bit(4) | bit(5) ),
		~( bit(3) | bit(5) ),
		~( bit(3) | bit(4) | bit(5) )
	};

	bool result = false;
	CriticalSectionEnter();

	if (!(semaphores & avoidMask[operation]))
	{
		bitSet(semaphores, operation);
		semaphores &= blockMask[operation];
		result = true;
	}

	CriticalSectionExit();
	return result;
}

void WiFiModuleESP::ReleaseSemaphore(uint8_t operation)
{
	CriticalSectionEnter();
	bitClear(semaphores, operation);
	CriticalSectionExit();
}

void WiFiModuleESP::CallbackNotFound(AsyncWebServerRequest* request)
{
	if (TryAcquireSemaphore(4))
	{
		bool compressed = false;
		bool canBeCached = false;
		String contentType;
		File file;
		String fileName;
	
		if (request->url().startsWith("/Assets/") && !request->url().endsWith("/"))
		{
			fileName = request->url();
			if (sdFat.exists(fileName.c_str()))					//
			{													//
				file.open(fileName.c_str(), O_READ);			// Posiadamy dokladnie taki plik o jaki prosi klient
				canBeCached = true;								//
				contentType = GetContentType(request->url());	//
			}													//
			else
			{
				fileName = request->url() + ".gz";
				if (sdFat.exists(fileName.c_str()))							//
				{															//
					file.open(fileName.c_str(), O_READ);					// Posiadamy plik o ktory prosi klient,
					compressed = true;										// ale w wersji skompresowanej
					canBeCached = true;										//
					contentType = GetContentType(request->url());			//
				}															//
				else
				{
					for (uint8_t i = 0; i < assetFileNames.size(); ++i)
					{
						if (request->url() == assetFileNames.get(i)->realName)					//
						{																		//
							fileName = assetFileNames.get(i)->shortName;						//
							file.open(fileName.c_str(), O_READ);								//
																								// Posiadamy plik o ktory prosi klient, ale pod inna nazwa
							if (fileName.endsWith(".gz"))										// ze wzgledu, ze SPIFFS dopuszcza tylko nazwy do 31 znakow
								compressed = true;												//
																								//
							canBeCached = true;													//
							contentType = GetContentType(request->url());						//
							break;																//
						}																		//
					}
				}
			}
		}
		else
		{
			if ((request->url().startsWith("/Logs/")) && !request->url().endsWith("/") && (serialListener.GetStateLogger() == serialListener.collectStopped))		//
			{																																						//
				fileName = request->url();																															//
				if (sdFat.exists(fileName.c_str()))																													//
				{																																					// Dostep do folderu z logami jest mozliwy
					file.open(fileName.c_str(), O_READ);																											// tylko gdy zbieranie logow jest zatrzymane
					contentType = GetContentType(request->url());																									//
				}																																					//
			}																																						//
			else																				//
			{																					//
				fileName = "/Htdocs";															//
				fileName += request->url();														//
																								//
				if (fileName.endsWith("/"))														//
				{																				//
					fileName += "index.html";													//
				}																				//
				else																			//
				{																				//
					if (!fileName.endsWith(".html"))											//
						fileName += ".html";													//
				}																				// Zakladamy, ze klient prosi o plik .html (strona)
																								//
				if (sdFat.exists(fileName.c_str()))												//
				{																				//
					file.open(fileName.c_str(), O_READ);										//
					contentType = GetContentType(fileName) + "; charset=UTF-8";					//
				}																				//
				else																			//
				{																				//
					fileName += ".gz";															//
					if (sdFat.exists(fileName.c_str()))											//
					{																			//
						file.open(fileName.c_str(), O_READ);									//
						compressed = true;														//
						contentType = GetContentType(fileName) + "; charset=UTF-8";				//
					}																			//
				}																				//
			}																					//
		}
	
		if (file)
		{
			#if DEBUG_WIFIMODULEESP
			String log;
			log += F("2_401\x1F");
			log += fileName;
			log += '\x1F';
			log += file.size();
			log += '\x1F';
			log += contentType;
			DEBUG_WIFIMODULEESP_LOG(log);
			#endif
	
			DownloadRequest* pDownloadRequest = new DownloadRequest(fileName);
			downloadRequests.add(pDownloadRequest);
			request->onDisconnect(std::bind(&WiFiModuleESP::CallbackNotFoundDisconnect, this, pDownloadRequest->key));
			AsyncWebServerResponse* response = request->beginResponse(contentType, file.size(), std::bind(&WiFiModuleESP::CallbackNotFoundFiller, this, pDownloadRequest->key, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
			file.close();

			if (compressed)
				response->addHeader("Content-Encoding", "gzip");
	
			if (canBeCached)
				response->addHeader("Cache-Control", "public, max-age=86400");
	
			request->send(response);
			return;
		}
		else
		{
			if (!downloadRequests.size())
				ReleaseSemaphore(4);
		}
	}

	request->send(404);							//
												//
	#if DEBUG_WIFIMODULEESP						//
	String log;									// W powyzszych instrukcjach nie udalo sie znalezc pliku o ktory prosi klient,
	log += F("2_402\x1F");						// a wiec zwracamy 404
	log += request->url();						//
	DEBUG_WIFIMODULEESP_LOG(log);				//
	#endif										//
}

size_t WiFiModuleESP::CallbackNotFoundFiller(uint16_t downloadRequestKey, uint8_t* buffer, size_t maxLen, size_t index)
{
	size_t bytesToSend = 0;
	if (TryAcquireSemaphore(4))
	{
		uint8_t downloadRequestIndex = 0;
		File file;
	
		for (; downloadRequestIndex < downloadRequests.size(); ++downloadRequestIndex)				//
		{																							//
			if (downloadRequests.get(downloadRequestIndex)->key == downloadRequestKey)				// Lista 'downloadRequests' przechowuje wszystkie aktualnie majace miejsce
			{																						// akcje wysylania plikow do klientow. Na podstawie 'downloadRequestKey' mozemy
				file.open(downloadRequests.get(downloadRequestIndex)->fileName.c_str(), O_READ);	// ustalic na rzecz ktorej z tych akcji zostala wywolana ta metoda
				break;																				//
			}																						//
		}																							//
	
		if (file)
		{
			if (downloadRequests.get(downloadRequestIndex)->justStarted)
			{
				downloadRequests.get(downloadRequestIndex)->justStarted = false;
			}
			else
			{
				if (index < file.size())
				{
					file.seekSet(index);
	
					if (file.available() >= maxLen)
					{
						bytesToSend = maxLen;
						file.read(buffer, bytesToSend);
					}
					else
					{
						bytesToSend = file.available();
						file.read(buffer, bytesToSend);
					}
	
					if (index + bytesToSend >= file.size())
					{
						#if DEBUG_WIFIMODULEESP			
						String log;
						log += F("2_431\x1F");
						log += downloadRequests.get(downloadRequestIndex)->fileName;
						DEBUG_WIFIMODULEESP_LOG(log);
						#endif
	
						delete downloadRequests.get(downloadRequestIndex);
						downloadRequests.remove(downloadRequestIndex);
	
						if (!downloadRequests.size())
							ReleaseSemaphore(4);
					}
				}
			}
	
			file.close();
		}
		else
		{
			if (downloadRequestIndex < downloadRequests.size())
			{
				delete downloadRequests.get(downloadRequestIndex);
				downloadRequests.remove(downloadRequestIndex);
			}
	
			if (!downloadRequests.size())
				ReleaseSemaphore(4);
	
			#if DEBUG_WIFIMODULEESP			
			String log;
			log += F("2_432");
			DEBUG_WIFIMODULEESP_LOG(log);
			#endif
		}
	}
	
	return bytesToSend;
}

void WiFiModuleESP::CallbackNotFoundDisconnect(uint16_t downloadRequestKey)
{
	for (uint8_t i = 0; i < downloadRequests.size(); ++i)
	{
		if (downloadRequests.get(i)->key == downloadRequestKey)
		{
			#if DEBUG_WIFIMODULEESP			
			String log;
			log += F("2_461");
			DEBUG_WIFIMODULEESP_LOG(log);
			#endif
	
			delete downloadRequests.get(i);
			downloadRequests.remove(i);
			break;
		}
	}
	
	if (!downloadRequests.size())
		ReleaseSemaphore(4);
}

void WiFiModuleESP::CallbackFormat(AsyncWebServerRequest* request)
{
	#if DEBUG_WIFIMODULEESP			
	String log;
	log += F("2_471");
	DEBUG_WIFIMODULEESP_LOG(log);
	#endif

	uint16_t* subMessage = new uint16_t();
	*subMessage = MESSAGE_FORMAT;
	timeKeeper.RequestMessage(millis(), this, (void**)subMessage);
	SuccessResponse(request);
}

void WiFiModuleESP::CallbackWeakFormat(AsyncWebServerRequest* request)
{
	#if DEBUG_WIFIMODULEESP			
	String log;
	log += F("2_481");
	DEBUG_WIFIMODULEESP_LOG(log);
	#endif

	uint16_t* subMessage = new uint16_t();
	*subMessage = MESSAGE_WEAK_FORMAT;
	timeKeeper.RequestMessage(millis(), this, (void**)subMessage);
	SuccessResponse(request);
}

void WiFiModuleESP::CallbackShutdown(AsyncWebServerRequest* request)
{
	CloseMassMemory();

	if (TryAcquireSemaphore(8))
	{
		#if DEBUG_WIFIMODULEESP			
		String log;
		log += F("2_491");
		DEBUG_WIFIMODULEESP_LOG(log);
		#endif

		uint16_t subMessage = MESSAGE_TURN_OFF_WIFI;
		serialListener.SendData(SERIAL_SOLARCONTROLLER, 2, (byte*)(&subMessage));
		SuccessResponse(request);
	}
	else
	{
		ErrorResponse(request);
	}
}

void WiFiModuleESP::CallbackRestart(AsyncWebServerRequest* request)
{
	CloseMassMemory();

	if (TryAcquireSemaphore(8))
	{
		#if DEBUG_WIFIMODULEESP			
		String log;
		log += F("2_501");
		DEBUG_WIFIMODULEESP_LOG(log);
		#endif

		uint16_t* subMessage = new uint16_t();
		*subMessage = MESSAGE_RESTART;
		timeKeeper.RequestMessage(millis() + 100, this, (void**)subMessage);
		SuccessResponse(request);
	}
	else
	{
		ErrorResponse(request);
	}
}

void WiFiModuleESP::CallbackUpload(AsyncWebServerRequest* request)
{
	if (!request->params())
	{
		String html;
		html += F(
		"<!DOCTYPE HTML>\n"
		"<html>\n"
		"\t<head>\n"
		"\t\t<meta charset=\"UTF-8\">\n"
		"\t\t<script src=\"/Assets/jquery-3.3.1.min.js\"></script>\n"
		"\t\t<script src=\"/Assets/UploadRequest.js\"></script>\n"
		"\t\t\n"
		"\t\t<script type=\"text/javascript\">\n"
		"\t\t\tfunction sendButtonClick(evt)\n"
		"\t\t\t{\n"
		"\t\t\t\tvar files = document.getElementById('fileToSend').files;\n"
		"\t\t\t\t\t\n"
		"\t\t\t\tif (files.length)\n"
		"\t\t\t\t{\n"
		"\t\t\t\t\tvar file = files[0];\n"
		"\t\t\t\t\tfileName = document.getElementById('path').value + file.name;\n"
		"\t\t\t\t\tvar fileReader = new FileReader();\n"
		"\t\t\t\t\tfileReader.onloadend = fileReaderLoadEnd;\n"
		"\t\t\t\t\tfileReader.readAsArrayBuffer(file);\n"
		"\t\t\t\t\tconsole.log(\"Rozpoczynam ladowanie pliku \" + file.name);\n"
		"\t\t\t\t}\n"
		"\t\t\t}\n"
		"\t\t\t\n"
		"\t\t\tfunction fileReaderLoadEnd(evt)\n"
		"\t\t\t{\n"
		"\t\t\t\tif (evt.target.readyState == FileReader.DONE)\n"
		"\t\t\t\t{\n"
		"\t\t\t\t\tUploadRequest.begin(fileName, evt.target.result, uploadRequestSuccess, uploadRequestError, uploadRequestProgress);\n"
		"\t\t\t\t}\n"
		"\t\t\t\telse\n"
		"\t\t\t\t{\n"
		"\t\t\t\t\talert('Nie udalo sie zaladowac pliku');\n"
		"\t\t\t\t\tfileName = null;\n"
		"\t\t\t\t}\n"
		"\t\t\t}\n"
		"\t\t\t\n"
		"\t\t\tfunction uploadRequestSuccess()\n"
		"\t\t\t{\n"
		"\t\t\t\talert('Udalo sie przeslac plik na serwer');\n"
		"\t\t\t\tprogress.innerHTML = \"\";\n"
		"\t\t\t\tfileName = null;\n"
		"\t\t\t}\n"
		"\t\t\t\n"
		"\t\t\tfunction uploadRequestError(errorString)\n"
		"\t\t\t{\n"
		"\t\t\t\tswitch(errorString)\n"
		"\t\t\t\t{\n"
		"\t\t\t\t\tcase \"1\":\n"
		"\t\t\t\t\t\talert('Nie udalo sia przeslac pliku. Nie mozna rozpoczac upload\\'u pliku');\n"
		"\t\t\t\t\t\tbreak;\n"
		"\t\t\t\t\t\t\n"
		"\t\t\t\t\tcase \"2\":\n"
		"\t\t\t\t\t\talert('Nie udalo sie przeslac pliku. Brakuje argumentow zadania');\n"
		"\t\t\t\t\t\tbreak;\n"
		"\t\t\t\t\t\t\n"
		"\t\t\t\t\tcase \"3\":\n"
		"\t\t\t\t\t\talert('Nie udalo sie przeslac pliku. Nie przeslano wszystkich czesci pliku');\n"
		"\t\t\t\t\t\tbreak;\n"
		"\t\t\t\t\t\t\n"
		"\t\t\t\t\tcase \"4\":\n"
		"\t\t\t\t\t\talert('Nie udalo sie przeslac pliku. Operacja zablokowana');\n"
		"\t\t\t\t\t\tbreak;\n"
		"\t\t\t\t\t\t\n"
		"\t\t\t\t\tcase \"101\":\n"
		"\t\t\t\t\t\talert('Nie udalo sie przeslac pliku. Nieznany blad');\n"
		"\t\t\t\t\t\tbreak;\n"
		"\t\t\t\t}\n"
		"\t\t\t\t\n"
		"\t\t\t\tprogress.innerHTML = \"\";\n"
		"\t\t\t\tfileName = null;\n"
		"\t\t\t}\n"
		"\t\t\t\n"
		"\t\t\tfunction uploadRequestProgress(bytesSent, bytesAll)\n"
		"\t\t\t{\n"
		"\t\t\t\tvar newHTML = \"Wyslano \";\n"
		"\t\t\t\tnewHTML += bytesSent;\n"
		"\t\t\t\tnewHTML += \"/\";\n"
		"\t\t\t\tnewHTML += bytesAll;\n"
		"\t\t\t\tnewHTML += \" bajtow\"\n"
		"\t\t\t\tprogress.innerHTML = newHTML;\n"
		"\t\t\t}\n"
		"\t\t\n"
		"\t\t\n"
		"\t\t\n"
		"\t\t\tvar progress;\n"
		"\t\t\tvar fileName;\n"
		"\t\t\n"
		"\t\t\n"
		"\t\t\n"
		"\t\t\t$( document ).ready(function()\n"
		"\t\t\t{\n"
		"\t\t\t\tif ( !(window.File && window.FileReader && window.FileList && window.Blob) )\n"
		"\t\t\t\t\talert('Obsluga plikow nie jest wspierana w tej przegladarce');\n"
		"\t\t\t\t\n"
		"\t\t\t\tdocument.getElementById('sendButton').addEventListener('click', sendButtonClick);\n"
		"\t\t\t\tprogress = document.getElementById(\"progress\");\n"
		"\t\t\t\tfileName = null;\n"
		"\t\t\t});\n"
		"\t\t\n"
		"\t\t</script>\n"
		"\t</head>\n"
		"\t<body>\n"
		"\t\t<p>Wybierz plik do przeslania</p>\n"
		"\t\t<input type=\"file\" id=\"fileToSend\"/>\n"
		"\t\t<p>Wpisz sciezke do ktorej ma zostac zapisany</p>\n"
		"\t\t<input type=\"text\" id=\"path\" value=\"/\">\n"
		"\t\t<button id=\"sendButton\">Przeslij</button>\n"
		"\t\t<p id=\"progress\"></p>\n"
		"\t</body>\n"
		"</html>"
		);
		request->send(200, "text/html", html);
		return;
	}
}

void WiFiModuleESP::CallbackUploadBodyHandler(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total)
{
	if (request->params())
	{
		if (TryAcquireSemaphore(3))
		{
			if (request->hasArg("fileName") && request->hasArg("currentByteIndex"))
			{
				if (!uploadRequest.active)
				{
					if (request->hasArg("fileSize"))
					{
						if (request->arg("fileName").length() > 254 || !request->arg("fileName").startsWith("/") || request->arg("currentByteIndex").toInt() != 0)	//
						{																																			//
							if (index + len == total)																												//
							{																																		//
								#if DEBUG_WIFIMODULEESP																												//
								String log;																															//
								log += F("2_561");																													//
								DEBUG_WIFIMODULEESP_LOG(log);																										// Nie mozna rozpoczac upload'u pliku
								#endif																																//
																																									//
								String additionalData = "1";																										//
								ErrorResponse(request, &additionalData);																							//
							}																																		//
																																									//
							ReleaseSemaphore(3);																													//
							return;																																	//
						}																																			//

						uploadRequest.BeginUpload(request->arg("fileName"), request->arg("fileSize").toInt());
					}
					else																								//
					{																									//
						if (index + len == total)																		//
						{																								//
							#if DEBUG_WIFIMODULEESP																		//
							String log;																					//
							log += F("2_562");																			//
							DEBUG_WIFIMODULEESP_LOG(log);																// Brakuje argumentow zadania
							#endif																						//
																														//
							String additionalData = "2";																//
							ErrorResponse(request, &additionalData);													//
						}																								//
																														//
						ReleaseSemaphore(3);																			//
						return;																							//
					}																									//
				}

				if ((uploadRequest.fileName != request->arg("fileName")) || (uploadRequest.file.size() != request->arg("currentByteIndex").toInt() + index))	//
				{																																				//
					uploadRequest.EndUpload();																													//
																																								//
					if (index + len == total)																													//
					{																																			//
						#if DEBUG_WIFIMODULEESP																													//
						String log;																																// Nie przeslano wszystkich czesci pliku
						log += F("2_563");																														//
						DEBUG_WIFIMODULEESP_LOG(log);																											//
						#endif																																	//
																																								//
						String additionalData = "3";																											//
						ErrorResponse(request, &additionalData);																								//
					}																																			//
																																								//
					ReleaseSemaphore(3);																														//
					return;																																		//
				}																																				//

				uploadRequest.file.write(data, len);

				if (uploadRequest.file.size() >= uploadRequest.targetFileSize)							//
				{																						//
					#if DEBUG_WIFIMODULEESP																//
					String log;																			//
					log += F("2_564\x1F");																//
					log += uploadRequest.fileName;														//
					log += '\x1F';																		//
					log += uploadRequest.file.size();													//
					DEBUG_WIFIMODULEESP_LOG(log);														//
					#endif																				//
																										// Prawidlowo odebrano czesc pliku od klienta,
					uploadRequest.EndUpload();															// byla to ostatnia czesc, transfer zakonczony
																										//
					if (index + len == total)															//
					{																					//
						String additionalData = "1";													//
						SuccessResponse(request, &additionalData);										//
					}																					//
																										//
					ReleaseSemaphore(3);																//
					return;																				//
				}																						//
				else																			//
				{																				//
					#if DEBUG_WIFIMODULEESP														//	
					String log;																	//
					log += F("2_565\x1F");														//
					log += uploadRequest.file.size();											//
					log += '\x1F';																//
					log += uploadRequest.targetFileSize;										// Prawidlowo odebrano czesc pliku od klienta,
					DEBUG_WIFIMODULEESP_LOG(log);												// czekamy na dalsze czesci
					#endif																		//
																								//
					if (index + len == total)													//
					{																			//
						String additionalData = "2";											//
						SuccessResponse(request, &additionalData);								//
					}																			//
																								//
					return;																		//
				}																				//
			}
			else																								//
			{																									//
				if (uploadRequest.active)																		//
					uploadRequest.EndUpload();																	//
																												//
				if (index + len == total)																		//
				{																								//
					#if DEBUG_WIFIMODULEESP																		//
					String log;																					// Brakuje argumentow zadania
					log += F("2_562");																			//
					DEBUG_WIFIMODULEESP_LOG(log);																//
					#endif																						//
																												//
					String additionalData = "2";																//
					ErrorResponse(request, &additionalData);													//
				}																								//
																												//
				ReleaseSemaphore(3);																			//
				return;																							//
			}																									//
		}
		else																							//
		{																								//
			if (index + len == total)																	//
			{																							//
				#if DEBUG_WIFIMODULEESP																	//
				String log;																				//
				log += F("2_567");																		// Operacja zablokowana
				DEBUG_WIFIMODULEESP_LOG(log);															//
				#endif																					//
																										//
				String additionalData = "4";															//
				ErrorResponse(request, &additionalData);												//
			}																							//
		}																								//
	}
}

void WiFiModuleESP::CallbackSetTime(AsyncWebServerRequest* request)
{
	if (request->hasArg("datetime") && (request->arg("datetime").length() == 16))
	{
		uint16_t year = request->arg("datetime").substring(0, 4).toInt();
		uint16_t month = request->arg("datetime").substring(5, 7).toInt();
		uint16_t day = request->arg("datetime").substring(8, 10).toInt();
		uint16_t hour = request->arg("datetime").substring(11, 13).toInt();
		uint16_t minute = request->arg("datetime").substring(14).toInt();
		uint16_t second = 0;
		setTime(hour, minute, second, day, month, year);
		SuccessResponse(request);
	}
	else
	{
		ErrorResponse(request);
	}
}

void WiFiModuleESP::CallbackSetNetworkCredentials(AsyncWebServerRequest* request)
{
	if (request->hasArg("ssid") && request->hasArg("password"))
	{
		config.ssid = request->arg("ssid");
		config.password = request->arg("password");
		uint16_t* subMessage = new uint16_t();
		*subMessage = MESSAGE_WRITE_CONFIG_FILE;
		timeKeeper.RequestMessage(millis(), this, (void**)subMessage);

		#if DEBUG_WIFIMODULEESP			
		String log;
		log += F("2_601\x1F");
		log += config.ssid;
		DEBUG_WIFIMODULEESP_LOG(log);
		#endif							

		SuccessResponse(request);
	}
	else
	{
		ErrorResponse(request);
	}
}

void WiFiModuleESP::CallbackSetApChannel(AsyncWebServerRequest* request)
{
	if (request->hasArg("channel") && request->arg("channel").toInt() > 0 && request->arg("channel").toInt() < 15)
	{
		config.apChannel = request->arg("channel");
		uint16_t* subMessage = new uint16_t();
		*subMessage = MESSAGE_WRITE_CONFIG_FILE;
		timeKeeper.RequestMessage(millis(), this, (void**)subMessage);

		#if DEBUG_WIFIMODULEESP			
		String log;
		log += F("2_631\x1F");
		log += config.apChannel;
		DEBUG_WIFIMODULEESP_LOG(log);
		#endif	

		SuccessResponse(request);
	}
	else
	{
		ErrorResponse(request);
	}
}

void WiFiModuleESP::CallbackNetworkScan(AsyncWebServerRequest* request)
{
	if (!BeginAsyncRequest(request))
	{
		ErrorResponse(request);
		return;
	}

	new NetworkScanTask(this);
	SuccessResponse(request);
}

void WiFiModuleESP::CallbackStartCollectingLogs(AsyncWebServerRequest* request)
{
	uint16_t* subMessage = new uint16_t();
	*subMessage = MESSAGE_START_COLLECTING_LOGS;
	timeKeeper.RequestMessage(millis(), this, (void**)subMessage);
	SuccessResponse(request);
}

void WiFiModuleESP::CallbackStopCollectingLogs(AsyncWebServerRequest* request)
{
	uint16_t* subMessage = new uint16_t();
	*subMessage = MESSAGE_STOP_COLLECTING_LOGS;
	timeKeeper.RequestMessage(millis(), this, (void**)subMessage);
	SuccessResponse(request);
}

void WiFiModuleESP::CallbackListDirectory(AsyncWebServerRequest* request)
{
	if (TryAcquireSemaphore(5))
	{
		if (request->hasArg("directory"))
		{
			char nameBuffer[256];
			File folderFile;
			File file;
			String folderName = request->arg("directory");
			folderName.replace('-', '/');
			String additionalString;

			if (folderFile.open(folderName.c_str(), O_READ) && folderFile.isDir())
			{
				folderFile.rewind();

				while (file.openNext(&folderFile, O_READ))
				{
					file.getName(nameBuffer, 256);
					String fileName = String(nameBuffer);

					if (file.isDir())
						fileName += '/';

					additionalString += fileName;
					additionalString += '\n';

					file.close();
				}
			}

			folderFile.close();

			if (additionalString.length())
				additionalString.remove(additionalString.length() - 1);

			SuccessResponse(request, &additionalString);
		}
		else											//
		{												//
			String additionalString = "1";				// Brakuje argumentow zadania
			ErrorResponse(request, &additionalString);	//
		}												//

		ReleaseSemaphore(5);
	}
	else												//
	{													//
		String additionalString = "2";					// Operacja zablokowana
		ErrorResponse(request, &additionalString);		//
	}													//
}

void WiFiModuleESP::CallbackSwitchToAuto(AsyncWebServerRequest* request)
{
	uint16_t subMessage = MESSAGE_AUTO;
	serialListener.SendData(SERIAL_SOLARCONTROLLER, 2, (byte*)(&subMessage));
	SuccessResponse(request);
}

void WiFiModuleESP::CallbackSwitchToManual(AsyncWebServerRequest* request)
{
	uint16_t subMessage = MESSAGE_MANUAL;
	serialListener.SendData(SERIAL_SOLARCONTROLLER, 2, (byte*)(&subMessage));
	SuccessResponse(request);
}

void WiFiModuleESP::CallbackGetStatus(AsyncWebServerRequest* request)
{
	if (!BeginAsyncRequest(request))
	{
		ErrorResponse(request);
		return;
	}

	uint16_t subMessage = MESSAGE_GET;
	serialListener.SendData(SERIAL_SOLARCONTROLLER, 2, (byte*)(&subMessage));
	SuccessResponse(request);
}

void WiFiModuleESP::CallbackSetParameters(AsyncWebServerRequest* request)
{
	if (!BeginAsyncRequest(request))
	{
		ErrorResponse(request);
		return;
	}

	if (request->hasArg("delay") && request->hasArg("sensitivity") && request->hasArg("overtake") && request->hasArg("nightThreshold") && request->hasArg("drag") && request->hasArg("overtakeVertical") && request->hasArg("overtakeHorizontal"))
	{
		byte dataPack[16];
		((short*)dataPack)[0] = MESSAGE_SET;
		((short*)dataPack)[1] = request->arg("delay").toInt();
		((short*)dataPack)[2] = request->arg("sensitivity").toInt();
		((short*)dataPack)[3] = request->arg("overtake").toInt();
		((short*)dataPack)[4] = request->arg("nightThreshold").toInt();
		((short*)dataPack)[5] = request->arg("drag").toInt();
		((short*)dataPack)[6] = request->arg("overtakeVertical").toInt();
		((short*)dataPack)[7] = request->arg("overtakeHorizontal").toInt();
		serialListener.SendData(SERIAL_SOLARCONTROLLER, 16, dataPack);
	}
	else
	{
		asyncRequest.locked = false;
		asyncRequest.result = "Wrong parameters";
	}

	SuccessResponse(request);
}

void WiFiModuleESP::CallbackControlMotor(AsyncWebServerRequest* request)
{
	if (!BeginAsyncRequest(request))
	{
		ErrorResponse(request);
		return;
	}

	if (request->hasArg("direction"))
	{
		byte dataPack[3];
		((short*)dataPack)[0] = MESSAGE_MOTOR;
		dataPack[2] = request->arg("direction").toInt();
		serialListener.SendData(SERIAL_SOLARCONTROLLER, 3, dataPack);
	}
	else
	{
		asyncRequest.locked = false;
		asyncRequest.result = "Wrong parameters";
	}

	SuccessResponse(request);
}

void WiFiModuleESP::CallbackAsyncRequestResult(AsyncWebServerRequest* request)
{
	if ( asyncRequest.locked || (asyncRequest.url == "") )
	{
		ErrorResponse(request);
	}
	else
	{
		String additionalData = asyncRequest.url;
		additionalData += '\n';
		additionalData += asyncRequest.invokeTime;
		additionalData += '\n';
		additionalData += asyncRequest.result;
		SuccessResponse(request, &additionalData);
		asyncRequest.Reset();
	}
}