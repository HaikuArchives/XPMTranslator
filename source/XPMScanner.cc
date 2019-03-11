//	XPMScanner.cc

#include "XPM.h"
#include "XPMScanner.h"

//	XPMScanner::XPMScanner(BPositionIO *)
//	accept pointer to a BPositionIO stream;
//	initialize 'buffer' and set 'index' to 0
XPMScanner::XPMScanner(BPositionIO *input)
{
	stream = input;
	index = length = 0;
	buffer[XPM_BUFFER_SIZE] = 0;				// ensure null-termination
}

//	XPMScanner::Setup(void)
//	fill 'buffer' from the stream for the first time.
status_t XPMScanner::Setup(void)
{
	status_t err;
	
	err = stream->Read(buffer,XPM_BUFFER_SIZE);
	if (err <= 0)
		return B_ERROR;
	else
	{
		index = 0;
		length = err;
		return B_OK;
	}
}

//	XPMScanner::GetLine(char *)
//	read in a complete line, demarcated either by a newline or a carriage return,
//	into 'string'.
status_t XPMScanner::GetLine(char *string)
{
	status_t err;
	int n, i = 0;
	bool wrapAround;
	bool done = false;
	
	while (!done)								// eat characters until the newline or
	{											// carriage return is reached
		n = strcspn(&buffer[index],"\r\n");
		strncpy(&string[i],&buffer[index],n);
		i += n;
		err = advance_n_chars(n,&wrapAround);
		if (err)
			return B_ERROR;
		if (!wrapAround)
			done = true;
	}
	
	done = false;
	while (!done)								// read past the newline(s) or
	{											// carriage return(s)
		n = strspn(&buffer[index],"\r\n");
		err = advance_n_chars(n,&wrapAround);
		if (err != B_OK)
			return B_ERROR;
		if (!wrapAround)
			done = true;
	}
		
	string[i] = 0;								// ensure null termination
	return B_OK;
}

//	XPMScanner::GetString(char *)
//	read complete strings, demarcated by double quotes, into 'string'.  'C' escaped
//	characters are ignored, since a proper XPM file is not likely to contain them.
status_t XPMScanner::GetString(char *string)
{
	status_t err;
	int n, i = 0;
	bool wrapAround;
	bool done = false;
	
	while (!done)								// eat characters until a quote is reached
	{
		n = strcspn(&buffer[index],"\"");
		err = advance_n_chars(n,&wrapAround);
		if (err)
			return B_ERROR;
		if (!wrapAround)
			done = true;
	}
	
	err = advance_n_chars(1,&wrapAround);		// eat the quote character
	if (err)
		return B_ERROR;
	
	done = false;
	while (!done)								// read characters until the closing quote
	{											// is found
		n = strcspn(&buffer[index],"\"");
		strncpy(&string[i],&buffer[index],n);
		i += n;
		err = advance_n_chars(n,&wrapAround);
		if (err)
			return B_ERROR;
		if (!wrapAround)
			done = true;
	}
	
	err = advance_n_chars(1,&wrapAround);		// eat the quote character
	if (err)
		return B_ERROR;

	string[i] = 0;
	return B_OK;
}

//	XPMScanner::advance_n_chars(int, bool *)
//	advance 'n' characters in the internal buffer.  If the end of the buffer be reached,
//	refresh the buffer from the stream and signal 'wrapAround' to warn the caller.
status_t XPMScanner::advance_n_chars(int n, bool *wrapAround)
{
	status_t err;
	
	index += n;
	if (index >= length)
	{
		*wrapAround = true;
		err = stream->Read(buffer,XPM_BUFFER_SIZE);
		if (err <= 0)
			return B_ERROR;
		else
		{
			index = 0;
			length = err;
		}	
	}
	else
		*wrapAround = false;
		
	return B_OK;
}
