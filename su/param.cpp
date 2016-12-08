#include "param.h"
#include "mbxchg.h"

#include <fstream>	
#include <sstream>	
#include <iostream>
#include <algorithm>
#include <string>
#include <string.h>

using namespace std;

std::vector< CParam, allocator<CParam> > params;

char easytolower(char in){
    if(in<='Z' && in>='A') return in-('Z'-'z');
    return in;
} 

// example of usage:
// removeCharsFromString( str, "()-" );
//
void removeCharsFromString( string &str, char* charsToRemove ) {
    for ( unsigned int i = 0; i < strlen(charsToRemove); ++i ) {
        str.erase( remove(str.begin(), str.end(), charsToRemove[i]), str.end() );
    }
    std::istringstream iss( str );
    std::getline( iss, str , '#');    // remove comment
    std::transform(str.begin(), str.end(), str.begin(), easytolower);
}


//  
//	Чтение и парсинг конфигурационного файла
//	name;mqtt;type;address;
//
int16_t readCfg()
{
	int16_t res=0;
    bool    fOutBlock = true;
    int16_t nI=0;

	std::fstream filestr("map.cfg");
    
    std::string line;
    while( std::getline( filestr, line ) ) {
        removeCharsFromString(line, (char *)" \t\r\n");
        if(fOutBlock && line[0]=='[') {
            if(line.find("modbusport") > 0 && line.find("commands")==std::npos) {

            }
            fOutBlock=false;
        }
        cout << line << endl;
/*        
        std::istringstream iss( line );
        std::string result;
        if( std::getline( iss, result , '=') ) {
            if( result == "foo" ) {
                std::string token;
                while( std::getline( iss, token, ',' ) ) {
                    std::cout << token << std::endl;
                }
            }
            if( result == "bar" ) {
            } 
        }
*/        
    }

	return res;
}


