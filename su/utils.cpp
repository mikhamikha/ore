#include "utils.h"

using namespace std;

int32_t dT = 0;

void setDT() {
    timespec    rawtime;
    struct tm   *ptm1;
    time_t      gt;

    clock_gettime(CLOCK_MONOTONIC, &rawtime);
   
    time(&gt);
    dT = int32_t(gt-rawtime.tv_sec);
    cout << " dT = " << dT << endl;
}


string to_string(int32_t i) {
    string s;
    stringstream out;
    out << i;
    s = out.str();
    return s;
}

string to_string(double i) {
    string s;
    stringstream out;
    out <<  i;
    s = out.str();
//    cout << "tostr "<<i<<" | "<<s<<endl;
    return s;
}

char easytolower(char in) {
    if(in<='Z' && in>='A') return in-('Z'-'z');
    return in;
} 

char easytoupper(char in){
    if(in<='z' && in>='a') return in+('Z'-'z');
    return in;
} 

// example of usage:
// removeCharsFromString( str, "()-" );
void removeCharsFromString( string &str, char* charsToRemove ) {
    for ( unsigned int i = 0; i < strlen(charsToRemove); ++i ) {
        str.erase( remove(str.begin(), str.end(), charsToRemove[i]), str.end() );
    }
    std::istringstream iss( str );
    std::getline( iss, str , '#');    // remove comment
}

void outtext(std::string tx) {
  time_t      rawtime;
//    timespec    rawtime;
    struct tm   *ptm;
    
      time(&rawtime);
//    clock_gettime(CLOCK_MONOTONIC, &rawtime);
//    ptm = localtime(&(rawtime.tv_sec));
    ptm = localtime(&rawtime);
    cout<<setfill('0')<<setw(2)<<ptm->tm_hour<<":"<<\
          setfill('0')<<setw(2)<<ptm->tm_min<<":"<< \
          setfill('0')<<setw(2)<<ptm->tm_sec<<"  "<<tx<<endl;
}

string time2string(time_t rawtime) {
    stringstream s;
    struct tm   *ptm;
    int32_t     dt = int32_t(rawtime);//+dT;

    ptm = localtime((time_t *)&dt);
  
    s <<    setfill('0')<<setw(4)<<ptm->tm_year+1900<<"/"<<  \
            setfill('0')<<setw(2)<<ptm->tm_mon+1<<"/"<<   \
            setfill('0')<<setw(2)<<ptm->tm_mday<<" "<< \
            setfill('0')<<setw(2)<<ptm->tm_hour<<":"<<  \
            setfill('0')<<setw(2)<<ptm->tm_min<<":"<<   \
            setfill('0')<<setw(2)<<ptm->tm_sec;

    return s.str();
}

string replaceString(string subject, const string& search, const string& replace) {
    size_t pos = 0;
    
    while((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
    return subject;
}
