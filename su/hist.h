//
// Исторический модуль
//
#ifndef _HIST_H
#define _HIST_H

#include <vector>
#include <iterator>
#include <stdint.h>
#include <string>
#include "utils.h"

#define _MAX_RECORDS    60000
#define _MAX_TAGS       1000
#define _TAG_LEN        4

#pragma pack(push, 1)

struct histCellHeader {
    uint32_t    m_size;                     // размер истории
    uint16_t    m_tagsize;                  // количество тэгов
    uint32_t    m_writeOff;                 // индекс записи
    char        m_tn[_MAX_TAGS][_TAG_LEN];  // массив имен тэгов
    int16_t     m_cn[_MAX_TAGS];            // номер внешнего соединения
};

struct histCellBody {
    double              m_value;            // значение тэга
    uint8_t             m_qual;             // качество
    int32_t             m_ts;               // временная метка в мс
    int16_t             m_id;
};
//
//  Ячейка истории
//
struct histCell {
    histCellHeader  m_h;
    histCellBody    m_b[_MAX_RECORDS];
};
#pragma pack(pop)

//
// исторический блок для одного тэга
//
struct histBlock {
    int16_t m_id;
    static int16_t m_cnt;
    histBlock() { } 
    histBlock( const histCellBody& val );
    ~histBlock() { /*cout<<"hb destr"<<endl;*/  }
    
    void    setvalue( histCellBody& val );
    int16_t getvalue( histCellBody& val );
};

//
//  класс исторического блока
//
class history : public cproperties<histBlock> {
//    static int32_t m_size;
//        histCell* m_pp;
        int16_t storageOperation( int16_t _typeOper, string& _n, histCellBody& _v, int32_t ref=-1 );
    public:
        history() { }
        int16_t init();
        int16_t clear();
        int16_t push( string& _n, double v, uint8_t q, int32_t t );
        int16_t push( string&, histCellBody& );
        int16_t pull( string& _n, histCellBody& _v, int32_t ref=-1 );
        string  hist_to_string( uint32_t );  
                        //  Перевод в строку содержимого ячейки памяти histCell с индексом uint16_t
};


extern history hist;

#endif
