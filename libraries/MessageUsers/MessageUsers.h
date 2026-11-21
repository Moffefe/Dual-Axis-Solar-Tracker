// Plik okreslajacy wiadomosci przesylane za pomoca interfejsu
// MessageReceiver w zakresie tego projektu

#ifndef MessageUsers_h
#define MessageUsers_h

#define MESSAGE_MANUAL			1001			// Przejdz w tryb reczny (WiFiModuleESP ---> SolarController)

#define MESSAGE_AUTO			1002			// Przejdz w tryb automatyczny (WiFiModuleESP ---> SolarController)

#define MESSAGE_GET				1003			// Prosze o wyslanie aktualnego statusu (WiFiModuleESP ---> SolarController)
#define MESSAGE_GET_RESPONSE	1004			// Aktualny status kontrolera solarnego (SolarController ---> WiFiModuleESP)

#define MESSAGE_SET				1005			// Chce zmodyfikowac twoje pola (WiFiModuleESP ---> SolarController)
#define MESSAGE_SET_RESPONSE	1006			// Informacja czy zmiany zostaly pomyslnie wprowadzone (SolarController ---> WiFiModuleESP)

#define MESSAGE_MOTOR			1007			// Chce sterowac mostkami H (WiFiModuleESP ---> SolarController)
#define MESSAGE_MOTOR_RESPONSE	1008			// Zgoda/odmowa sterowania mostkami H (SolarController ---> WiFiModuleESP)

#define MESSAGE_TURN_OFF_WIFI	1009			// Wylacz modul WiFi (WiFiModuleESP ---> SolarController)



#define MESSAGE_TURN_ON_WIFI	2001			// Wlacz modol WiFi (SolarController ---> TimeKeeper ---> SolarController)



#define MESSAGE_CONNECT_TASK_END			3001			// Zadanie laczenia z siecia zostalo zakonczone (ConnectTask ---> WiFiModuleESP)
#define MESSAGE_NETWORK_SCAN_TASK_END		3002				// Zadanie skanowania sieci zostalo zakonczone (NetworkScanTask ---> WiFiModuleESP)

#define MESSAGE_START_COLLECTING_LOGS		3003					// Wywolaj StartCollectingLogs() (WiFiModuleESP ---> TimeKeeper ---> WiFiModuleESP)
#define MESSAGE_STOP_COLLECTING_LOGS		3004						// Wywolaj StopCollectingLogs() (WiFiModuleESP ---> TimeKeeper ---> WiFiModuleESP)
#define MESSAGE_WRITE_CONFIG_FILE			3005			// Wywolaj WriteConfigFile() (WiFiModuleESP ---> TimeKeeper ---> WiFiModuleESP)
#define MESSAGE_RESTART						3006				// Wywolaj ESP.restart() (WiFiModuleESP ---> TimeKeeper ---> WiFiModuleESP)
#define MESSAGE_FORMAT						3007					// Zaplanuj pelny format pamieci (WiFiModuleESP ---> TimeKeeper ---> WiFiModuleESP)
#define MESSAGE_WEAK_FORMAT					3008						// Zaplanuj oczyszczanie pamieci (WiFiModuleESP ---> TimeKeeper ---> WiFiModuleESP)

#define MESSAGE_CHECK_WIFI_CONNECTION		3009							// Sprawdz czy nadal jestesmy podlaczeni do AP (WiFiModuleESP ---> TimeKeeper ---> WiFiModuleESP)

#endif