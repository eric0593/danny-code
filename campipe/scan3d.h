#ifndef __SCAN3D_H__
#define __SCAN3D_H__


class scan3d
{
public:
	void loadimage();
	void loadYuvBuf(char *buf_l,char *buf_r,int framenum,int framesize);
	bool readBinCalData(char* fileName);
	void printCloud(char* fileName);
    void decMulPha5();
	void configScan3d();

	void * pMultiPhase;
	double m_dFreq[5];
	int    m_nBinThre;
	float m_fMaxAngle;
};


extern "C"  
{ 
scan3d* getScan3d(void);
void freetScan3d(scan3d *);
}
#endif
