var UploadRequest =
{
	fileName : null,
	currentByteIndex : null,
	partSize : null,
	fileBytes : null,
	jxhr : null,
	success : null,
	error : null,
	progress : null,
	
	begin : function(fileName, fileBytes, success = null, error = null, progress = null)
	{
		this.fileName = fileName;
		this.currentByteIndex = 0;
		this.partSize = 1000;
		this.fileBytes = fileBytes;
		this.success = success;
		this.error = error;
		this.progress = progress;
		
		var fileBytesPart;
					
		if(this.currentByteIndex + this.partSize < this.fileBytes.byteLength)
			fileBytesPart = new Uint8Array(this.fileBytes, this.currentByteIndex, this.partSize);
		else
			fileBytesPart = new Uint8Array(this.fileBytes, this.currentByteIndex);
		
		var urlString = "upload?fileName=";
		urlString += this.fileName;
		urlString += "&currentByteIndex=";
		urlString += this.currentByteIndex;
		urlString += "&fileSize=";
		urlString += this.fileBytes.byteLength;
		
		this.jxhr = $.ajax({
			url : urlString,
			type : 'POST',
			contentType: 'application/octet-stream',
			data : fileBytesPart,
			processData: false,
			dataType : 'text',
			success : $.proxy(this.ajaxFilePartSendSuccess, this),
			error : $.proxy(this.ajaxFilePartSendError, this)
		});
		
		if(this.progress != null)
			this.progress(this.currentByteIndex, this.fileBytes.byteLength);
	},
	
	ajaxFilePartSendSuccess : function(text, textStatus, jxhr)
	{
		if(this.jxhr == jxhr)
		{
			var lines = text.split(/\n/);
					
			if(lines[0] == "Error")
			{
				if(this.error != null)
					this.error(lines[1]);
				
				this.clean();
				return;
			}
		
			this.currentByteIndex += this.partSize;
			
			if(this.currentByteIndex >= this.fileBytes.byteLength)
			{
				if(this.success != null)
					this.success();
				
				this.clean();
			}
			else
			{
				var fileBytesPart;
						
				if(this.currentByteIndex + this.partSize < this.fileBytes.byteLength)
					fileBytesPart = new Uint8Array(this.fileBytes, this.currentByteIndex, this.partSize);
				else
					fileBytesPart = new Uint8Array(this.fileBytes, this.currentByteIndex);
				
				var urlString = "upload?fileName=";
				urlString += this.fileName;
				urlString += "&currentByteIndex=";
				urlString += this.currentByteIndex;
				
				this.jxhr = $.ajax({
					url : urlString,
					type : 'POST',
					contentType: 'application/octet-stream',
					data : fileBytesPart,
					processData: false,
					dataType : 'text',
					success : $.proxy(this.ajaxFilePartSendSuccess, this),
					error : $.proxy(this.ajaxFilePartSendError, this)
				});
				
				if(this.progress != null)
					this.progress(this.currentByteIndex, this.fileBytes.byteLength);
			}
		}
	},
	
	ajaxFilePartSendError : function(jxhr, status)
	{
		if(this.jxhr == jxhr)
		{
			if(this.error != null)
				this.error("101");
			
			this.clean();
		}
	},
	
	clean : function()
	{
		this.fileName = null;
		this.currentByteIndex = null;
		this.partSize = null;
		this.fileBytes = null;
		this.jxhr = null;
		this.success = null;
		this.error = null;
		this.progress = null;
	}
}