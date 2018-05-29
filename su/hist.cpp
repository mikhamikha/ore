#include "hist.h"
#include <sys/mman.h>
#include <fcntl.h>

history hist;

//int32_t history::m_size = 0;            // инициализируем начальный размер истории
int16_t histBlock::m_cnt = 0;           // инициализируем начальное количество тэгов для истории
//histCell* history::m_phl = NULL;

//
//  Переведем в строку содержимое ячейки памяти с индексом i
//
string history::hist_to_string(uint32_t i) {
    stringstream    _out("");
    string          _n;
    histCellBody    _hc;
    
    if ( pull(_n, _hc, i)==_exOK ) {
        _out<<"tn="<<_n
            <<" val="<<_hc.m_value
            <<" q=0x"<<hex<<uint16_t(_hc.m_qual)<<dec
            <<" ts="<<_hc.m_ts;
    }
    return _out.str(); 
}

histBlock::histBlock( const histCellBody& val ) {
    histCellBody* phc = (histCellBody*)&val;
//    cout<<"hb constr 1"<<endl;
    m_id = m_cnt++;                           
    setvalue( *phc );
}

void histBlock::setvalue( histCellBody& val ) {
    cout<<"histBlock::setvalue histCell_size="<<sizeof(histCellBody)+sizeof(histCellHeader)<<endl;
    val.m_id = m_id;                            // сохраним id тэга
}

int16_t histBlock::getvalue( histCellBody& val ) {
    int16_t rc= ( !m_id ) ? _exBadParam : _exOK;
    
    if( !rc ) {
//        val = m_ohl.back();
//        m_ohl.pop_back();
    }
    return rc;
}
//
//  Функция работы со статической памятью
//  _typeOper   - тип операции (0 - сохранение, 1 - чтение, 2 - инициализация, 3 - очистка истории) 
//  _name       - имя тэга    
//  _val        - ячейка данных
//  _ind        - индекс в массиве истории
//
int16_t history::storageOperation( int16_t _typeOper, string& _name, histCellBody& _val, int32_t _ind ) {
    int16_t rc = _exOK;
    int fd = -1;
    
    // Открываем статическую память
    fd = open("/dev/sram", O_RDWR|O_SYNC, S_IRUSR|S_IWUSR);
    if (fd == -2) {
        cout<<"history::push Error open static memory /dev/sram\n";
        rc = fd;
    }     
    if( !rc ) {
        // Маппируем статическую память
        int _ns = sizeof(histCell);
    //    cout<<" histCellSize="<<_ns<<endl;
        
        histCell* m_pp = (histCell*) mmap( 0, _ns, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0 );
        if ( m_pp == MAP_FAILED )
            cout<<"history::push mmap error\n";
        else {
    //        cout<<"mmap ok\t"<<m_off<<endl;
            switch( _typeOper ) {
                case 0:     // запись в историю
                    memcpy( &m_pp->m_b[m_pp->m_h.m_writeOff++], &_val, sizeof(histCellBody) );
                    m_pp->m_h.m_size += ( m_pp->m_h.m_size < _MAX_RECORDS-1 )? 1: 0;
                    m_pp->m_h.m_writeOff = (m_pp->m_h.m_writeOff) % _MAX_RECORDS;
                    
                    if( setproperty( _name, _val )!=_exOK ) {       // если тэга еще не было в истории
                        memset( m_pp->m_h.m_tn[_val.m_id], 0, _TAG_LEN ); // сохраним имя в заголовке 
                        memcpy( m_pp->m_h.m_tn[_val.m_id], _name.c_str(), min( _TAG_LEN, int(_name.length()) ) ); 
                    }
                    break;
                case 1:     // чтение из истории
                    //int i = _ind; 
                    if( m_pp->m_h.m_size && _ind>=-1 && _ind<_MAX_RECORDS ) {   
                        if( _ind==-1 ) {
                            m_pp->m_h.m_size--;
                            _ind = m_pp->m_h.m_writeOff? m_pp->m_h.m_writeOff-1: _MAX_RECORDS-1;
                            m_pp->m_h.m_writeOff = _ind; 
                        }
                        _name = string( m_pp->m_h.m_tn[m_pp->m_b[_ind].m_id], _TAG_LEN );
                        memcpy( (void *)(&_val), (void*)(m_pp->m_b+_ind), sizeof(histCellBody) ); 
                    }
                    else rc = _exBadParam;
                    break;
                case 2:     // инициализация при старте
                    for(uint16_t j=0; j<m_pp->m_h.m_size; j++) {
                        string _n = string( m_pp->m_h.m_tn[j], _TAG_LEN );
                        histCellBody _v;   
                        setproperty( _n, _v );
                    }
                    break;                
                case 3:     // очистка истории
                    m_pp->m_h.m_size = 0;
                    break;
            }
            munmap( m_pp, _ns );
        }
        close(fd);
    }
    return rc;
}
//
//  сохраним значение параметра в истории
//
int16_t history::push( string& _n, histCellBody& _v ) {
    return storageOperation( 0, _n, _v, 0 );
}

int16_t history::push( string& _n, double v, uint8_t q, int32_t t ) {
    histCellBody hc = { v, q, t, 0 };
    return push( _n, hc );
}
//
//  получим значение записанного параметра из истории по номеру записи
// 
int16_t history::pull( string& _n, histCellBody& _v, int32_t ref ) {
    int16_t rc = storageOperation( 1, _n, _v, ref );   
    
    return rc;
}
//
//  Устанавливаем доступ к истории
// 
int16_t history::init() {
    int16_t rc = _exOK;
    string          _n;
    histCellBody    _v;
    storageOperation( 2, _n, _v, 0 );

    return rc;
}
//
//  Очищаем историю
// 
int16_t history::clear() {
    int16_t rc = _exOK;
    string          _n;
    histCellBody    _v;
    storageOperation( 3, _n, _v, 0 );
    clearPropertyList();

    return rc;
}
