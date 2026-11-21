var LoggerClient = 
{
	logsTimelineDiv : null,
	logsTimeline : null,
	logsGraph1Div : null,
	logsGraph1 : null,
	logsGraph2Div : null,
	logsGraph2 : null,
	logsGraph3Div : null,
	logsGraph3 : null,
	
	dateFrom : null,
	timeFrom : null,
	dateTo : null,
	timeTo : null,
	noTimeLogs : null,
	classView : null,
	timelineSlider : null,
	
	logsDefinitions : null,
	
	noTimeLogsState : null,
	stringFrom : null,
	stringTo : null,
	filter : null,
	sensorMap : null,
	files : null,
	timezoneOffset : null,
	
	init : function(logsTimelineDiv, logsGraph1Div, logsGraph2Div, logsGraph3Div, dateFrom, timeFrom, dateTo, timeTo, noTimeLogs, classView, timelineSlider, logsDefinitions, submitButton)
	{
		this.logsTimelineDiv = logsTimelineDiv;
		this.logsGraph1Div = logsGraph1Div;
		this.logsGraph2Div = logsGraph2Div;
		this.logsGraph3Div = logsGraph3Div;
		this.dateFrom = dateFrom;
		this.timeFrom = timeFrom;
		this.dateTo = dateTo;
		this.timeTo = timeTo;
		this.noTimeLogs = noTimeLogs;
		this.classView = classView;
		this.timelineSlider = timelineSlider;
		this.logsDefinitions = logsDefinitions;
		
		submitButton.click($.proxy(this.submitButtonClick, LoggerClient));
		this.fillClassView();
	},
	
	submitButtonClick : function()
	{
		if(ContentBodyManager.currentDownload != null)
			return;
		
		this.noTimeLogsState = this.noTimeLogs[0].checked;
		
		if(!this.noTimeLogsState)
		{
			if(this.dateFrom.html() == "" || this.timeFrom.html() == "" || this.dateTo.html() == "" || this.timeTo.html() == "")
				return;
		
			let dateSeparator = null;
			if(this.dateFrom.html().indexOf('/') != -1)
			{
				dateSeparator = '/';
			}
			else
			{
				if(this.dateFrom.html().indexOf('.') != -1)
				{
					dateSeparator = '.';
				}
			}
			
			this.stringFrom = this.dateFrom.html().split(dateSeparator);
			
			if(this.stringFrom[0].length == 1)
				this.stringFrom[0] = '0' + this.stringFrom[0];
			
			if(this.stringFrom[1].length == 1)
				this.stringFrom[1] = '0' + this.stringFrom[1];
		
			this.stringFrom = this.stringFrom[2] + '-' + this.stringFrom[1] + '-' + this.stringFrom[0] + 'T' + this.timeFrom.html() + ':00';
			
			this.stringTo = this.dateTo.html().split(dateSeparator);
			
			if(this.stringTo[0].length == 1)
				this.stringTo[0] = '0' + this.stringTo[0];
			
			if(this.stringTo[1].length == 1)
				this.stringTo[1] = '0' + this.stringTo[1];
			
			this.stringTo = this.stringTo[2] + '-' + this.stringTo[1] + '-' + this.stringTo[0] + 'T' + this.timeTo.html() + ':00';
			
			let tempDateFrom = new Date(this.stringFrom);
			let tempDateTo = new Date(this.stringTo);
			
			if( tempDateFrom >= tempDateTo || ((tempDateTo.getDate() - tempDateFrom.getDate()) > 2) )
				return;
			
			this.stringFrom = this.stringFrom.replace(/-|T|:/g, '');
			this.stringTo = this.stringTo.replace(/-|T|:/g, '');
			
			this.timezoneOffset = new Date().getTimezoneOffset() * 1000 * 60;
		}
		else
		{
			this.stringFrom = '19700101000000';
			this.stringTo = '19700105000000';
			
			this.timezoneOffset = 0;
		}
		
		this.stringFrom += '.txt';
		this.stringTo += '.txt';
		
		this.filter = this.getFilter();
		this.sensorMap = [];
		
		let logsCount = 0;
		let sensorLogsCount = 0;
		for(let i = 1; i < this.filter.length; ++i)
		{
			for(let j = 1; j < this.filter[i].length; ++j)
			{
				if(this.filter[i][j] != null)
				{
					++logsCount;
					
					if(this.filter[i][j][0] == 32)
					{
						if(typeof this.sensorMap[i] === 'undefined')
							this.sensorMap[i] = [];
						
						this.sensorMap[i][j] = sensorLogsCount;
						++sensorLogsCount;
					}
				}
			}
		}
		
		if(logsCount == 0 || sensorLogsCount > 3)
			return;
		
		ContentBodyManager.currentDownload = 'a';
		ContentBodyManager.throbber.removeClass('no-visible');
		
		if(this.logsTimeline != null)
		{
			this.logsTimeline.destroy();
			this.logsTimeline = null;
		}
		
		if(this.logsGraph1 != null)
		{
			this.logsGraph1.destroy();
			this.logsGraph1 = null;
		}
		
		if(this.logsGraph2 != null)
		{
			this.logsGraph2.destroy();
			this.logsGraph2 = null;
		}
		
		if(this.logsGraph3 != null)
		{
			this.logsGraph3.destroy();
			this.logsGraph3 = null;
		}
		
		this.timelineSlider.addClass('d-none');
		
		$.ajax(
		{
			url : '/stop-collecting-logs',
			type : 'GET',
			dataType : 'text',
			success : $.proxy(this.ajaxStopCollectingLogsSuccess, this),
			error : $.proxy(this.clean, this)
		});
	},
	
	fillClassView : function()
	{
		let html = '<ul data-role="treeview">';
		let className = null;
		let logType = null;
		let logName = null;
		
		let i = 1;
		for (; i < this.logsDefinitions.length; ++i)
		{
			className = this.logsDefinitions[i][0];
			html += '<li><input data-role="checkbox" data-caption="';
			html += className;
			html += '"><ul class="expanded">'
			
			let j = 1;
			for (; j < this.logsDefinitions[i].length; ++j)
			{
				if(typeof this.logsDefinitions[i][j] === 'undefined')
					continue;
				
				logType = this.logsDefinitions[i][j][0];
				logName = this.logsDefinitions[i][j][1];
				html += '<li><input data-role="checkbox" data-caption="';
				
				switch(logType)
				{
					case 0:
					case 8:
					case 16:
					case 24:
						if(logName.indexOf(' ') != -1)
						{
							html += logName.substr(0, logName.indexOf(' '));
							html += '...';
						}
						else
						{
							html += logName;
						}
						break;
						
					case 32:
						html += logName;
						break;
				}
				
				html += '"></li>';
			}
			
			html += '</ul></li>';
		}
		
		html += '</ul>';
		this.classView.append(html);
	},
	
	getFilter : function()
	{
		let result = [];
		let inputArray = this.classView.find('input');
		let inputArrayIndex = 0;
		
		let i = 1;
		for(; i < this.logsDefinitions.length; ++i)
		{
			++inputArrayIndex;
			result[i] = [];
			
			let j = 1;
			for (; j < this.logsDefinitions[i].length; ++j)
			{
				if(typeof this.logsDefinitions[i][j] === 'undefined')
					continue;
				
				if(inputArray[inputArrayIndex].checked)
					result[i][j] = this.logsDefinitions[i][j];
				else
					result[i][j] = null;
				
				++inputArrayIndex;
			}
		}
		
		return result;
	},
	
	ajaxStopCollectingLogsSuccess : function(text)
	{
		let lines = text.split(/\n/);
		if(lines[0] == "Error")
		{
			this.clean();
			return;
		}
		
		setTimeout(function()
		{
			let urlString = '/list-directory?directory=Logs'
			
			if(LoggerClient.noTimeLogsState)
				urlString += '-NoTime';
			
			$.ajax(
			{
				url : urlString,
				type : 'GET',
				dataType : 'text',
				success : $.proxy(LoggerClient.ajaxListDirectorySuccess, LoggerClient),
				error : $.proxy(LoggerClient.clean, LoggerClient)
			});
		},
		100);
	},
	
	ajaxListDirectorySuccess : function(text)
	{
		let lines = text.split(/\n/);
		if(lines[0] == "Error")
		{
			this.clean();
			return;
		}
		
		lines.shift();
		this.files = [];
		
		if(this.noTimeLogsState)
		{
			let pathPrefix = '/Logs/NoTime/';
			
			for(let i = 0; i < lines.length; ++i)
				this.files.push([false, pathPrefix + lines[i], ""]);
		}
		else
		{
			let linesTemp = [];
			let pathPrefix = '/Logs/';
			
			for(let i = 0; i < lines.length; ++i)
			{
				if(!lines[i].endsWith('/'))
					linesTemp.push(pathPrefix + lines[i]);
			}
			
			linesTemp.sort(this.compareLogNames);
			
			let i = 0;
			for (; i < linesTemp.length; ++i)
			{
				if (this.compareLogNames(this.stringTo, linesTemp[i]) == -1)
					break;
			}
			
			for (; i < linesTemp.length; ++i)
			{
				this.files.push([false, linesTemp[i], ""]);
				
				if (this.compareLogNames(this.stringFrom, linesTemp[i]) == -1)
					break;
			}
			
			if(lines.indexOf('b/') != -1)
			{
				let urlString = '/list-directory?directory=Logs-b'
				
				$.ajax(
				{
					url : urlString,
					type : 'GET',
					dataType : 'text',
					success : $.proxy(this.ajaxListSubdirectorySuccess, this),
					error : $.proxy(this.clean, this)
				});
				
				return;
			}
		}
		
		if(this.files.length == 0)
		{
			this.clean();
			return;
		}
		
		for(let i = 0; i < this.files.length; ++i)
		{
			$.ajax(
			{
				url : this.files[i][1],
				type : 'GET',
				dataType : 'text',
				success : LoggerClient.ajaxLogDownloadSuccess,
				error : LoggerClient.ajaxLogDownloadError
			});
		}
	},
	
	ajaxListSubdirectorySuccess : function(text)
	{
		let lines = text.split(/\n/);
		if(lines[0] == "Error")
		{
			this.clean();
			return;
		}
		
		lines.shift();
		
		let pathPrefix = '/Logs/b/';
		
		for(let i = 0; i < lines.length; ++i)
			lines[i] = pathPrefix + lines[i];
		
		lines.sort(this.compareLogNames);
		
		let i = 0;
		for (; i < lines.length; ++i)
		{
			if (this.compareLogNames(this.stringTo, lines[i]) == -1)
				break;
		}
		
		for (; i < lines.length; ++i)
		{
			this.files.push([false, lines[i], ""]);
			
			if (this.compareLogNames(this.stringFrom, lines[i]) == -1)
				break;
		}
		
		if(this.files.length == 0)
		{
			this.clean();
			return;
		}
		
		for(let i = 0; i < this.files.length; ++i)
		{
			$.ajax(
			{
				url : this.files[i][1],
				type : 'GET',
				dataType : 'text',
				success : LoggerClient.ajaxLogDownloadSuccess,
				error : LoggerClient.ajaxLogDownloadError
			});
		}
	},
	
	ajaxLogDownloadSuccess : function(text, textStatus, jxhr)
	{	
		let completeFilesCount = 0;
		for(i = 0; i < LoggerClient.files.length; ++i)
		{
			if(LoggerClient.files[i][1] == this.url)
			{
				LoggerClient.files[i][0] = true;
				LoggerClient.files[i][2] = text;
			}
			
			if(LoggerClient.files[i][0])
				++completeFilesCount;
		}
		
		if(completeFilesCount == LoggerClient.files.length)
			LoggerClient.showLogs();
	},
	
	ajaxLogDownloadError : function()
	{
		let completeFilesCount = 0;
		for(i = 0; i < LoggerClient.files.length; ++i)
		{
			if(LoggerClient.files[i][1] == this.url)
				LoggerClient.files[i][0] = true;
			
			if(LoggerClient.files[i][0])
				++completeFilesCount;
		}
		
		if(completeFilesCount == LoggerClient.files.length)
			LoggerClient.showLogs();
	},
	
	showLogs : function()
	{
		let itemId = 0;
		let timelineItems = [];
		let graphsItems = [];
		graphsItems[0] = [];
		graphsItems[1] = [];
		graphsItems[2] = [];
		let graphGroups = new vis.DataSet();
		graphGroups.add({id : 0, content : 'Gr0', style : 'color: blue'});
		graphGroups.add({id : 1, content : 'Gr1', style : 'color: red'});
		graphGroups.add({id : 2, content : 'Gr2', style : 'color: yellow'});
		graphGroups.add({id : 3, content : 'Gr3', style : 'color: green'});
		
		for(let i = 0; i < this.files.length; ++i)
		{
			let lines = this.files[i][2].split(/\n/);
			
			let j = lines.length - 1;
			for(; j >= 0; --j)
			{
				if(lines[j].length < 18)
					continue;
				
				if (this.compareLogNames(this.stringTo, lines[j], false) == -1)
					break;
			}
			
			for(; j >= 0; --j)
			{
				if(lines[j].length < 18)
					continue;
				
				if (this.compareLogNames(this.stringFrom, lines[j], false) == -1)
					break;
				
				let classNumber = null;
				let underscoreIndex = lines[j].indexOf('_', 15);
				if(underscoreIndex == -1)
					continue;
				classNumber = lines[j].substring(15, underscoreIndex);
				classNumber = parseInt(classNumber);
				
				if(isNaN(classNumber))
					continue;
				
				let logNumber = null;
				let unitSepIndex = lines[j].indexOf('', underscoreIndex + 1);
				if(unitSepIndex == -1)
				{
					logNumber = parseInt(lines[j].substr(underscoreIndex + 1));
				}
				else
				{
					logNumber = lines[j].substring(underscoreIndex + 1, unitSepIndex);
					logNumber = parseInt(logNumber);
				}
				if(isNaN(logNumber))
					continue;
				
				if(typeof this.filter[classNumber][logNumber] === 'undefined')
				{
					continue
				}
				else
				{
					if(this.filter[classNumber][logNumber] != null)
					{
						if(this.filter[classNumber][logNumber][0] == 32)
						{
							let logArguments = lines[j].substr(unitSepIndex + 1).split(//);
							
							if(typeof logArguments[0] !== 'undefined')
								graphsItems[this.sensorMap[classNumber][logNumber]].push({id : itemId++, x : this.getLogDateUTC(lines[j], false), y : logArguments[0], group : 0});
							
							if(typeof logArguments[1] !== 'undefined')
								graphsItems[this.sensorMap[classNumber][logNumber]].push({id : itemId++, x : this.getLogDateUTC(lines[j], false), y : logArguments[1], group : 1});
							
							if(typeof logArguments[2] !== 'undefined')
								graphsItems[this.sensorMap[classNumber][logNumber]].push({id : itemId++, x : this.getLogDateUTC(lines[j], false), y : logArguments[2], group : 2});
							
							if(typeof logArguments[3] !== 'undefined')
								graphsItems[this.sensorMap[classNumber][logNumber]].push({id : itemId++, x : this.getLogDateUTC(lines[j], false), y : logArguments[3], group : 3});
						}
						else
						{
							let logArguments = lines[j].substr(unitSepIndex + 1).split(//);
							let content = this.logsDefinitions[classNumber][0];
							content += '<br>';
							content += this.sprintf(this.filter[classNumber][logNumber][1], logArguments);
							timelineItems.push({id : itemId++, content: content, start: this.getLogDateUTC(lines[j], false)});
						}
					}
				}
			}
		}

		let timelineOptions =
		{
			start : this.getLogDateUTC(this.stringFrom),
			end : this.getLogDateUTC(this.stringFrom) + 1000 * 10,
			zoomMax: 1000 * 60,
			height : 450
		};
		
		let graphOptions =
		{
			start : this.getLogDateUTC(this.stringFrom),
			end : this.getLogDateUTC(this.stringTo),
			interpolation: false,
			height : 300,
			legend : true
		};
		
		if(timelineItems.length > 0)
		{
			let timelineDataset = new vis.DataSet(timelineItems);
			this.logsTimeline = new vis.Timeline(this.logsTimelineDiv[0], timelineDataset, timelineOptions);
			
			this.timelineSlider.removeClass('d-none');
			this.logsTimeline.on('rangechange', function()
			{
				if(!arguments[0].byUser)
					return;
				
				let newPosition = LoggerClient.map(LoggerClient.getLogDateUTC(LoggerClient.stringFrom), LoggerClient.getLogDateUTC(LoggerClient.stringTo), 0, 255, arguments[0].start.getTime() + ((arguments[0].end.getTime() - arguments[0].start.getTime()) / 2))
				LoggerClient.timelineSlider.find('input')[0].value = newPosition;
			});
			
			this.timelineSlider.find('input').on('input', function()
			{
				let newPosition = LoggerClient.map(0, 255, LoggerClient.getLogDateUTC(LoggerClient.stringFrom), LoggerClient.getLogDateUTC(LoggerClient.stringTo), this.value);
				LoggerClient.logsTimeline.moveTo(newPosition, false);
			});
		}
		
		if(graphsItems[0].length > 0)
		{
			let graphDataset = new vis.DataSet(graphsItems[0]);
			this.logsGraph1 = new vis.Graph2d(this.logsGraph1Div[0], graphDataset, graphGroups, graphOptions);
			this.logsGraph1.on('rangechange', function(){this.redraw();});
		}
		
		if(graphsItems[1].length > 0)
		{
			let graphDataset = new vis.DataSet(graphsItems[1]);
			this.logsGraph2 = new vis.Graph2d(this.logsGraph2Div[0], graphDataset, graphGroups, graphOptions);
			this.logsGraph2.on('rangechange', function(){this.redraw();});
		}
		
		if(graphsItems[2].length > 0)
		{
			let graphDataset = new vis.DataSet(graphsItems[2]);
			this.logsGraph3 = new vis.Graph2d(this.logsGraph3Div[0], graphDataset, graphGroups, graphOptions);
			this.logsGraph3.on('rangechange', function(){this.redraw();});
		}
		
		this.clean();
	},
	
	clean : function()
	{
		$.ajax(
		{
			url : '/start-collecting-logs',
			type : 'GET',
			dataType : 'text'
		});
		
		ContentBodyManager.throbber.addClass('no-visible');
		ContentBodyManager.currentDownload = null;
	},
	
	sprintf : function(format, argumentsArray)
	{
		let placeholders = format.match(/%s/g);
		
		if(placeholders != null && placeholders.length != argumentsArray.length)
			return null;
		
		let i = 0;
		return format.replace(/%s/g, () => argumentsArray[i++]);
	},
	
	map : function( from1, to1, from2, to2, v )
	{
        return from2 + ( v - from1 ) * ( to2 - from2 ) / ( to1 - from1 );
    },
	
	compareLogNames : function(a, b, isFileName = true)
	{
		if(isFileName)
		{
			let aOffset = a.length - 18;
			let bOffset = b.length - 18;
			
			for (i = 0; i < 14; ++i)
			{
				if (a.charAt(aOffset + i) > b.charAt(bOffset + i))
				{
					return -1;
				}
				else
				{
					if (a.charAt(aOffset + i) < b.charAt(bOffset + i))
					{
						return 1;
					}
				}
			}
		}
		else
		{
			for (i = 0; i < 14; ++i)
			{
				if (a.charAt(i) > b.charAt(i))
				{
					return -1;
				}
				else
				{
					if (a.charAt(i) < b.charAt(i))
					{
						return 1;
					}
				}
			}
		}
		
		return 0;
	},
	
	getLogDateISO : function(log, isFileName = true)
	{
		if(isFileName)
		{
			let offset = log.length - 18;
			let result = log.substr(offset, 4);
			result += '-';
			result += log.substr(offset + 4, 2);
			result += '-';
			result += log.substr(offset + 6, 2);
			result += 'T';
			result += log.substr(offset + 8, 2);
			result += ':';
			result += log.substr(offset + 10, 2);
			result += ':';
			result += log.substr(offset + 12, 2);
			return result;
		}
		else
		{
			let result = log.substr(0, 4);
			result += '-';
			result += log.substr(4, 2);
			result += '-';
			result += log.substr(6, 2);
			result += 'T';
			result += log.substr(8, 2);
			result += ':';
			result += log.substr(10, 2);
			result += ':';
			result += log.substr(12, 2);
			return result;
		}
	},
	
	getLogDateUTC : function(log, isFileName = true)
	{
		let result = null;
		
		if(isFileName)
		{
			let offset = log.length - 18;
			result = Date.UTC(parseInt(log.substr(offset, 4)), parseInt(log.substr(offset + 4, 2)) - 1, parseInt(log.substr(offset + 6, 2)), parseInt(log.substr(offset + 8, 2)), parseInt(log.substr(offset + 10, 2)), parseInt(log.substr(offset + 12, 2)));
		}
		else
		{
			result = Date.UTC(parseInt(log.substr(0, 4)), parseInt(log.substr(4, 2)) - 1, parseInt(log.substr(6, 2)), parseInt(log.substr(8, 2)), parseInt(log.substr(10, 2)), parseInt(log.substr(12, 2)));
		}
		
		result += this.timezoneOffset;
		return result;
	}
}