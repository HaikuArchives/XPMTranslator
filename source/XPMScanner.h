//	XPMScanner.h
//	a class for buffered reading-in of entire lines and quoted strings
//	from XPM files.

#ifndef XPM_SCANNER_H
#define XPM_SCANNER_H

#define		XPM_BUFFER_SIZE		4096

class XPMScanner
{
	public:
	
		XPMScanner(BPositionIO *);

		status_t Setup(void);		
		status_t GetLine(char *);
		status_t GetString(char *);
		
	private:
	
		status_t advance_n_chars(int, bool *);
		
		BPositionIO *stream;
		char buffer[XPM_BUFFER_SIZE+1];
		int length;
		int index;
};

#endif 