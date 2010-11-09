#include "indent.h"

#define PROCESS_EVENT 0
#define IGNORE_EVENT  1

static PluginStartupInfo Info;
static HKEY hKeyIndentExt=NULL;

extern "C" {

BOOL WINAPI __stdcall LibMain( HINSTANCE inst, DWORD reason, LPVOID data ){
	return 1;
}

/*-----==< EXPORTED FUNCTIONS >==-----*/
void __stdcall SetStartupInfo(struct PluginStartupInfo * _Info){
    Info=*_Info;
	DBG(flog = fopen("indent.log","w"));
	DBG(fputs("SetStartupInfo\n",flog));
	RegCreateKey(HKEY_CURRENT_USER,"Software\\Far\\Plugins\\Indent\\ext",&hKeyIndentExt);
}

void __stdcall ExitFAR(){
	DBG(fputs("ExitFAR\n",flog));
	DBG(fflush(flog));
	DBG(if(flog)fclose(flog));
}

void __stdcall GetPluginInfo(struct PluginInfo *_Info){
	DBG(fputs("GetPluginInfo\n",flog));
    _Info->StructSize=sizeof(*_Info);
    _Info->Flags=PF_EDITOR|PF_DISABLEPANELS;
    _Info->DiskMenuStringsNumber=0;
    _Info->PluginMenuStrings=0;
    _Info->PluginMenuStringsNumber=0;
    _Info->PluginConfigStringsNumber=0;
}

HANDLE __stdcall OpenPlugin(int OpenFrom,int Item){
    return(INVALID_HANDLE_VALUE);
}

int __stdcall ProcessEditorInput(INPUT_RECORD *Rec){
    WORD vkey;
    DWORD ks;

    if (Rec->EventType!=KEY_EVENT) return PROCESS_EVENT;
    if (!Rec->Event.KeyEvent.bKeyDown) return PROCESS_EVENT;

    char ch=Rec->Event.KeyEvent.uChar.AsciiChar;
    vkey=Rec->Event.KeyEvent.wVirtualKeyCode;
    ks=Rec->Event.KeyEvent.dwControlKeyState;

	//DBG(fprintf(flog,"ProcessEditorInput vKey=%i keyState=%i char=%c\n",vkey,ks,(ch==0?1:ch) ));
		
    ks&=~(CAPSLOCK_ON|ENHANCED_KEY|NUMLOCK_ON|SCROLLLOCK_ON);
	if(vkey==VK_TAB){
		if ( !(ks & SHIFT_PRESSED) ) return IndentBlock();
		else if ( ks & SHIFT_PRESSED ) return UnIndentBlock();
	}else if(vkey==VK_RETURN){
		return Autoindent();
	}else if(ch=='}'){	//vkey==221 && ks & SHIFT_PRESSED){	// ']','}' button
		return ProcessCloseBracket();
	}

	return PROCESS_EVENT;
}


}

/*-----==< PROCESSING FUNCTIONS >==-----*/
int IndentBlock(){
    struct EditorInfo ei;
    struct EditorGetString gs;
    struct EditorSetString ss;
	int line;
	int ret=PROCESS_EVENT;
	char*c;

    Info.EditorControl(ECTL_GETINFO,&ei);

    if (ei.BlockType==BTYPE_STREAM){
		DBG(fprintf(flog,"IndentBlock\n"));
		DBG(fflush(flog));
		line=ei.BlockStartLine;
		while(1){
			if(ei.TotalLines<=line)break;
			gs.StringNumber=line;
			if(!Info.EditorControl(ECTL_GETSTRING,&gs))break;
			if(gs.SelStart<0)break;
			if(gs.StringLength>0&&gs.SelEnd!=0){
				ret=IGNORE_EVENT;
				c=new char[gs.StringLength+1];
				DBG(fprintf(flog,"IndentBlock new char[%i]\n",gs.StringLength+1));
				DBG(fflush(flog));
				c[0]='\t';
				ss.StringLength=MyStrCpy(c+1,gs.StringText,gs.StringLength,&ei)+1;

				ss.StringNumber=line;
				ss.StringText=c;
				ss.StringEOL=gs.StringEOL;
				DBG(fprintf(flog,"IndentBlock SETSTRING\n"));
				DBG(fflush(flog));
				Info.EditorControl(ECTL_SETSTRING,&ss);
				delete []c;
				ExpandTabs(&ei,line);
			}
			line++;
		}
		DBG(fprintf(flog,"IndentBlock REDRAW\n"));
		DBG(fflush(flog));
		Info.EditorControl(ECTL_REDRAW,NULL);
	}
	return ret;
}

int UnIndentBlock(){
    EditorInfo ei;
    EditorGetString gs;
    EditorSetString ss;
	int line;
	int ret=PROCESS_EVENT;


    Info.EditorControl(ECTL_GETINFO,&ei);

    if (ei.BlockType==BTYPE_STREAM){
		DBG(fprintf(flog,"UnIndentBlock\n"));
		DBG(fflush(flog));
		line=ei.BlockStartLine;
		while(1){
			if(ei.TotalLines<=line)break;
			gs.StringNumber=line;
			if(!Info.EditorControl(ECTL_GETSTRING,&gs))break;
			if(gs.SelStart<0)break;
			if(gs.StringLength>0&&gs.SelEnd!=0){
				ret=IGNORE_EVENT;
				int tabsize = GetTabSize(gs.StringText,ei.TabSize);
				if(tabsize>0){
					ss.StringNumber=line;
					ss.StringText=gs.StringText+tabsize;
					ss.StringEOL=gs.StringEOL;
					int length=gs.StringLength-tabsize;
					if(length<0)length=0;
					ss.StringLength=length;
					Info.EditorControl(ECTL_SETSTRING,&ss);
					ExpandTabs(&ei,gs.StringNumber);
				}
			}
			line++;
		}
		Info.EditorControl(ECTL_REDRAW,NULL);
	}
	return ret;
}
int ProcessCloseBracket(){
    EditorInfo ei;
    EditorSetString ss;

    Info.EditorControl(ECTL_GETINFO,&ei);
	if(ei.Options&EOPT_AUTOINDENT){
		int line=ei.CurLine;
		if(line>0){
			char * ext=GetExtension(ei.FileName);
			if( CompatibleExtension(ext) ){
				EditorGetString gs,fgs;
				gs.StringNumber=line;
				Info.EditorControl(ECTL_GETSTRING,&gs);
				if(EmptyString(gs.StringText,gs.StringLength)){
					//looking for opened bracket
					int brCount=0;
					while( (--line)>=0 ){
						fgs.StringNumber=line;
						Info.EditorControl(ECTL_GETSTRING,&fgs);
						if(BracketOpened(fgs.StringText)){
							if(brCount>0)brCount--;
							else{
								DBG(fprintf(flog,"ProcessCloseBracket\n"));
								DBG(fflush(flog));
								int fch=GetFirstChar(fgs.StringText,fgs.StringLength);
								char *c=new char[fch+2];
								strncpy(c,fgs.StringText,fch);
								c[fch]='}';
								c[fch+1]=0;
								ss.StringNumber=gs.StringNumber;
								ss.StringText=c;
								ss.StringEOL=NULL;
								ss.StringLength=fch+1;
								Info.EditorControl(ECTL_SETSTRING,&ss);
								delete []c;
								ExpandTabs(&ei,gs.StringNumber);
								EditorSetPosition pos;
								pos.CurLine=-1;
								pos.CurPos=fch+1;
								pos.CurTabPos=-1;
								pos.TopScreenLine=-1;
								pos.LeftPos=-1;
								pos.Overtype=-1;
								Info.EditorControl(ECTL_SETPOSITION,&pos);
								Info.EditorControl(ECTL_REDRAW,NULL);
								return IGNORE_EVENT;
							}
						}else{
							int fch=GetFirstChar(fgs.StringText,fgs.StringLength);
							if(fch>=0 && fgs.StringText[fch]=='}')brCount++;
						}
					}
				}
			}
		}
	}
	return PROCESS_EVENT;
}


int Autoindent(){
    EditorInfo ei;
    Info.EditorControl(ECTL_GETINFO,&ei);
	int ret=PROCESS_EVENT;
	if(ei.Options&EOPT_AUTOINDENT){
		DBG(fprintf(flog,"Autoindent\n"));
		DBG(fflush(flog));

		ret=IGNORE_EVENT;
		Info.EditorControl(ECTL_INSERTSTRING,NULL);
		
		EditorGetString gs,fgs;
		gs.StringNumber=-1;
		Info.EditorControl(ECTL_GETSTRING,&gs);
		int line=ei.CurLine;
		int fch=-1;
		while(line>=0){
			fgs.StringNumber=line;
			Info.EditorControl(ECTL_GETSTRING,&fgs);
			fch=GetFirstChar(fgs.StringText,fgs.StringLength);
			if(fch>=0){
				DBG(fprintf(flog,"Autoindent GetFirstChar(s,%i)=%i\n",fgs.StringLength,fch));
				DBG(fflush(flog));
				break;
			}
			line--;
		}
		if(line>=0&&fch>=0){
			DBG(fprintf(flog,"Autoindent prepare string to set\n"));
			DBG(fflush(flog));
			char *c=new char[fch+1+1];
			strncpy(c,fgs.StringText,fch);
			if(BracketOpened(fgs.StringText)){
				char * ext=GetExtension(ei.FileName);
				if( CompatibleExtension(ext) )c[fch++]='\t';
			}
			c[fch]=0;
			Info.EditorControl(ECTL_INSERTTEXT,c);
			delete []c;
			ExpandTabs(&ei,-1);
			Info.EditorControl(ECTL_REDRAW,NULL);
		}
	}
	return ret;
}

/*-----==< TOOL FUNCTIONS >==-----*/

/**
 * returns the number of symbols for first tab for specified string
 */
int GetTabSize(char *text,int tabsize){
	int i=0;
	for(i=0;i<tabsize;i++){
		if(text[i]=='\t')return i+1;
		if(text[i]!=' ')return i;
	}
	return i;
}

/**
 * returns true if string contains only space characters
 */
bool EmptyString(char *text,int length){
	for(int i=0;i<length;i++){
		char c=text[i];
		if(c=='\t'||c==' ')continue;
		else return false;
	}
	return true;
	
}

/**
 * Returns position of the first non space character
 */
int GetFirstChar(char *text,int length){
	for(int i=0;i<length;i++){
		if(text[i]=='\t'||text[i]==' ')continue;
		return i;
	}
	return -1;
}

/**
 * Returns extension of the file name
 */
char*GetExtension(char*fname){
	char * ext=NULL;
	for(int i=0;fname[i]!=0;i++){
		char c=fname[i];
		if(c=='.')ext=fname+i+1;
		else if(c=='/'||c=='\\')ext=NULL;
	}
	return ext;
}

/**
 * Returns true if last nonspace character is '{'
 */
bool BracketOpened(char*s){
	bool b=false;
	for(int i=0;s[i]!=0;i++){
		char c=s[i];
		if(c=='{')b=true;
		else if(c==' '||c=='\t')continue;
		else b=false;
	}
	return b;
}

/**
 * Returns true if we need to process '{' and '}'
 */
bool CompatibleExtension(char*ext){
	if(!ext)return false;
	DWORD len,type;
	if(RegQueryValueEx(hKeyIndentExt,ext,NULL,&type,NULL,&len)==ERROR_SUCCESS)return true;
	return !strcmpi(ext,"java") || !strcmpi(ext,"c") || !strcmpi(ext,"cpp") ;
}

/**
 * Expands Tabs to spaces in specified line if this option is on
 */
void ExpandTabs(EditorInfo*ei,int line){
	DBG(fprintf(flog,"ExpandTabs EOPT_EXPANDTABS=%i line=%i\n",ei->Options&EOPT_EXPANDTABS,line));
	DBG(fflush(flog));
	DBG(int res);
	if(ei->Options&EOPT_EXPANDTABS)
		DBG(res=)Info.EditorControl(ECTL_EXPANDTABS,&line);
	DBG(fprintf(flog,"ExpandTabs Result=%i\n",res));
	DBG(fflush(flog));
}

/**
 * Copies src to dst and if EOPT_EXPANDTABS is not specified, converts 
 *  first spaces to tabs.
 * Returns dst string length
 */
int MyStrCpy(char*dst,char*src,int srclen,EditorInfo*ei){
	DBG(fprintf(flog,"MyStrCpy srclen=%i\n",srclen));
	DBG(fflush(flog));
	if(ei->Options&EOPT_EXPANDTABS){
		DBG(fprintf(flog,"MyStrCpy expand tabs\n"));
		DBG(fflush(flog));
		memcpy(dst,src,srclen);
		return srclen;
	}else{
		DBG(fprintf(flog,"MyStrCpy tabs\n"));
		DBG(fflush(flog));
		int dstpos=0;
		int scount=0;
		int srcpos=0;
		for(int i=0;i<srclen;i++){
			if(src[i]==' ')scount++;
			else if(src[i]=='\t')scount=ei->TabSize;
			else break;
			if(scount==ei->TabSize){
				srcpos=i+1;
				scount=0;
				dst[dstpos++]='\t';
			}
		}
		if(srcpos<srclen)
			memcpy(dst+dstpos,src+srcpos,srclen-srcpos);
		DBG(fprintf(flog,"MyStrCpy length=%i\n",dstpos+srclen-srcpos));
		DBG(fflush(flog));
		return dstpos+srclen-srcpos;
	}
}

