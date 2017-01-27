#include "display.h"

using namespace std;

view dsp;

//
// функция заполнения данных класса дисплея конфигурационными данными
//
void view::definedspline( int16_t nd, int16_t nr, std::string sr) {
    if( nd > 0 && nr > 0 ) {
        if( pages.size() < nd ) {
            pagestruct  page;
            while( pages.size() < nd ) pages.push_back(page);
        }
        if( pages[nd-1].rows.size() < nr ) {
            string s("");
            while( pages[nd-1].rows.size() < nr ) pages[nd-1].rows.push_back(s);
        }
        pages[nd-1].rows[nr-1] = sr;
        cout<<"define dsp "<<nd<<" row "<<nr<<" content "<<sr \
            <<" dspsize "<<pages.size()<<" rows "<<pages[nd-1].rows.size()<<endl;
    }
}

//
// перевод кодировки русских символов в CP866  
//
void view::to_866(string& sin, string& sout) {
    uint16_t    n, j;
    wchar_t     ws[2];    
    char        ss[2];
//    int i = 0;
    vector<char>buf;

    for(j=0; j<sin.length(); ) {
//        printf("%d=%d>", j, sin[j]);
        if(sin[j]<208) {
            buf.push_back(sin[j]);
//            *(*sout+i) = sin[j];
            ++j;
        } 
        else if(j<sin.length()-2){
            ss[1]=sin[j]; 
            ss[0]=sin[j+1];
            n = *((int16_t *)ss);
    
            if(n>=53392 && n<=53439) {
                n = n-53264;
            }
            else if(n>=53632 && n<=53647) {
                n = n-53408;
            }
            else if(n==53377) {
                n = 240;
            }
            else if(n==53649) {
                n = 241;
            }
            buf.push_back(n);
            j += 2;
        }
//        ++i;
    }
    string s(buf.begin(), buf.end());
    sout = s;
}

void view::print( string& sin ) {
    string buf, son("{"), soff("}");
    to_866( sin, buf );
    assignValues(buf, son, soff);
//    cout<<buf<<endl;
    Noritake_VFD_GU3000::println(buf.c_str());
}

//
// функция вывода сконфигурированного кадра на дисплей
//
void view::outview(int16_t ndisp) {
    int16_t     nd = (ndisp>0)? ndisp-1: 0;
    int16_t     nr;
    pagestruct  &pg = pages[nd];
    
//    cout<<"outview( "<<ndisp<<" ) pages "<<pages.size()<<" rows "<<pg.rows.size()<<endl;

//    GU3000_clearScreen();
//    GU3000_setCursor( 0, 0 );
    for(nr = 0; nr < pg.rows.size(); ++nr) {
        string sout;
        (!nr)?GU3000_boldOn():GU3000_boldOff();
//        cout<<pg.rows[nr].c_str()<<endl;
        sout = pg.rows[nr];
        sout.append( 10u, ' ' );
        GU3000_setCursor( 0, nr*11 );
        print(sout);
//        GU3000_clearLineEnd();
    }
//    cout<<endl;

/*    int16_t i,j,k;
    
   k=0;
    for(i=0; i<8; i++)
    for(j=0; j<32; j++) {
        GU3000_setCursor(j*8, i*17);
        print(k++);
    } 
*/
}

//
// функция замены тэгов в строках {tag} на их значения
//
int16_t assignValues(string& subject, const string& sop, const string& scl) {
    size_t      nbe = 0, nen = 0;
    int16_t     rc=EXIT_SUCCESS;
    bool        fgo = true;

//    cout<<subject<<" "<<subject.length()<<endl;
    while(fgo) {
        nbe = subject.find(sop, nen); 
        fgo = (nbe != std::string::npos);
        if( fgo && (nen = subject.find(scl, nbe)) != std::string::npos ) {
                string na(subject, nbe+1, nen-nbe-1);
                string va;
//                cout<<" beg="<<nbe<<" end="<<nen<<" tag="<<na<<" | ";
                if(getparam( na, va ) == EXIT_SUCCESS) {
                    subject.replace( nbe, nen-nbe+1, va );
                    nen = nbe+va.length();
                }
        }
        else fgo =false;
//        cout<<fgo<<" "<<subject<<" "<<subject.length()<<endl;
    }
//    cout<<endl;
    return rc;
}

//
// поток выдачи изображения на дисплей 
//
void* viewProcessing(void *args) 
{
    while (fParamThreadInitialized) {
        dsp.outview(1);
        usleep(100000);
    }
    
    return EXIT_SUCCESS;
}
