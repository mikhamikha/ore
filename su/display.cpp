#include "display.h"

view dsp;

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
/*        cout<<"define dsp "<<nd<<" row "<<nr<<" content "<<sr \
            <<" dspsize "<<pages.size()<<" rows "<<pages[nd-1].rows.size()<<endl;*/
    }
}

void view::outview(int16_t ndisp) {
    int i,j,k;
    GU3000_init();
    GU3000_setCharset(CP866);
 
    GU3000_setFontSize(_8x16Format,1,1);
    GU3000_setScreenBrightness(20);
    k=0;
    for(i=0; i<8; i++)
    for(j=0; j<32; j++) {
        GU3000_setCursor(j*8, i*17);
        print(k++);
    }
}
