#include "utils.h"

using namespace std;

string to_string(int32_t i) {
    string s;
    stringstream out;
    out << i;
    s = out.str();
    return s;
}

char easytolower(char in){
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
//    std::transform(str.begin(), str.end(), str.begin(), easytolower);
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

