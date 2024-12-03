#ifndef CMARKER_GSM_H_
#define CMARKER_GSM_H_

#define   BUFF_SZ             32u
#define   SKIP_CHAR           0xFFu

// Declaration part
class CMarkerGsm
{
public:
   static char Buf[BUFF_SZ];
   static char Buf2[BUFF_SZ];
   
   CMarkerGsm(const char* m, const char* b, const char e, bool rl) : 
        marker(m), mrk_len(strlen(marker)), match(0u),  
		state(eNOT_DETECTED), beg(b), end(e), bReadLine(rl)  {};
   void check(const char ch);
   void reset();
   bool isReady() const;
   
private:
   enum eState {eNOT_DETECTED = 0, eDETECTED, eDATA_COLLECTING, eDATA_COLLECTING_LINE, eREADY};
   
   const char* marker;
   const byte mrk_len;
   byte  match;
   eState state;
   const char* beg;
   const char  end;
   bool  bReadLine;
};


// Implementation part
char CMarkerGsm::Buf[BUFF_SZ] = {0};
char CMarkerGsm::Buf2[BUFF_SZ] = {0};


void CMarkerGsm::check(const char ch)
{
    switch(state)
    {
        case eNOT_DETECTED: // Try to detect the marker in a byte stream
          if (ch == marker[match]) ++match;
          else match = 0u;
          if (mrk_len == match)
          {
              state = eDETECTED;
              match = 0u;
          }
          break;

        case eDETECTED: // The marker has been detected
          if (ch == beg[match]) ++match;
          else match = 0u;
          if (strlen(beg) == match)
          {
              state = eDATA_COLLECTING;
              match = 0u;
          }
          break;

        case eDATA_COLLECTING: // Collect the expected data (phone numder, SMS index)
          if ((end != ch) && (match < (BUFF_SZ - 1u)))  Buf[match++] = ch;
          else if (true == bReadLine)
          {
              state = eDATA_COLLECTING_LINE;
              match = SKIP_CHAR;
          }
          else state = eREADY;
          break;

       case eDATA_COLLECTING_LINE: // Optional: read SMS body
          if ('\n' == ch)
          {
              if (SKIP_CHAR == match) match = 0u;
              else state = eREADY;
          }
          else if(match < (BUFF_SZ - 1u))  Buf2[match++] = ch;
          break;
          
      case eREADY: // Do nothing
          break;
    }
}

void CMarkerGsm::reset()
{
    state = eNOT_DETECTED;
    memset(Buf, 0, BUFF_SZ);
    memset(Buf2, 0, BUFF_SZ);
    match = 0u;
}

bool CMarkerGsm::isReady() const
{
	return CMarkerGsm::eREADY == state;
}

#endif