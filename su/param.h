// заголовочный файл param.h
#ifndef _PARAM_H
	#define _PARAM_H
#include <vector>
#include <string>	
#include <ctime>
#include <stdint.h>

// интерфейс класса
// объявление класса Параметр
class CParam 			// имя класса
{
private: 				// спецификатор доступа private
protected: 				// спецификатор доступа protected
    
    time_t 			timestamp;
	unsigned int 	quality;
	double			dValue;
	bool			bValue;
	unsigned int	id;	
public: 				// спецификатор доступа public
    CParam(){};			// конструктор класса
    ~CParam(){};   
	std::string	name;
	void 		setValue();
    void 		getValue(); 				// 
}; // конец объявления класса CParam


int16_t readCfg();
void* fieldXChange(void *args);    // поток обмена по Modbus с полевым оборудованием

extern std::vector< CParam, std::allocator<CParam> > params;


#endif // _PARAM_H
