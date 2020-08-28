//-------------------------------------------------
// Shenzhen TOPWAY Technology Co.,Ltd.
// LCD Module:        LMT043DFFFWD-NEA
// System:            W78E516D(12MHz, 6T)
// Display Size:      480(RGB)x272
// Driver/Controller: S1D13L01
// Interface:         I/O  SPI mode  
// Date:	            2018-07-18
// by                 lu  jianhui               
// note:              PLL mode 10M CLKI 54M MCLK  9M PCLK
//-------------------------------------------------
#include <reg52.h>
#include <intrins.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define uchar unsigned char 	// 8bit, 0 ~  255
#define uint  unsigned int  	// 16bit, 0 ~  65,535
#define ulong unsigned long 	// 32bit, 0 ~  4,294,967,295

//-----------------------------------
// define I/O
//-----------------------------------
#define LCDBUS_H  P1

    sbit	A0        = P2^0;
    sbit  _RST      = P2^2;
    sbit  TE        = P2^3;
    sbit  _RD       = P2^4;
    sbit  _LB       = P2^6;
    sbit  _UB       = P2^7;
    sbit  BL_ADJ    = P3^7;
    sbit   SPI_EN   = P3^6;
    sbit   _CS      = P2^1;
    sbit   SCLK     = P2^5;		     
    sbit   SI       = P0^0;
    sbit   SO       = P0^1;
    sbit   D2       = P0^2;
    sbit   D3       = P0^3;
    sbit   D4       = P0^4;
    sbit   D5       = P0^5;
    sbit   D6       = P0^6;
    sbit   D7       = P0^7;
    sbit _TP_CS     = P3^1;        // TP.CS
    sbit TP_DCLK    = P3^0;
    sbit TP_DIN     = P3^2;
    sbit TP_DOUT    = P3^4;
    sbit _TP_PENQ   = P3^5;        // 0 = touched

//----------------------------------
//  触摸屏驱动子程序
    sbit TPENIRQ    = P3^5;
    sbit TDOUT      = P3^4;
    sbit TBUSY      = P3^3;
    sbit TDIN       = P3^2;
    sbit T_CS       = P3^1;
    sbit TDCLK      = P3^0;
    uchar bdata transdata;         //该变量可为位操作之变量
    sbit transbit7  = transdata^7;
    sbit transbit0  = transdata^0;

//-----------------------------------
    bit  AutoRun;                   // 1 for auto run, 0 for manual run

    uchar bdata btemp;              // create a bit accessable byte
    sbit btemp_b0   = btemp^0;
    sbit btemp_b1   = btemp^1;
    sbit btemp_b2   = btemp^2;
    sbit btemp_b3   = btemp^3;
    sbit btemp_b4   = btemp^4;
    sbit btemp_b5   = btemp^5;
    sbit btemp_b6   = btemp^6;
    sbit btemp_b7   = btemp^7;
//

uint  TP_x;                     // TP data after touch
uint  TP_y; 
uchar TpCheck;
uchar code TDisplay[]={0xC3,0x66,0x3C,0x18,0x18,0x3C,0x66,0xC3,};

 //uint txdata,tydata;
//-----------------------------------
// define 16bit colors
//-----------------------------------
#define RED     0xf800
#define GREEN   0x07e0
#define BLUE    0x001f
#define YELLOW  0xffe0
#define CYAN    0x07ff
#define MAGENTA 0xf81f
#define BLACK   0x0000
#define WHITE   0xffff
#define GRAY04  0x4208

//-----------------------------------------------------------------------------
// delayms routine
// Parameter：m = time in ms
//-----------------------------------------------------------------------------
void delayms(uint m)                  // 12MHz Crystal, close to ms value
{
    uint j;
    uint i;
    for(i=0; i<m; i++)
        for(j=0; j<109; j++)
            _nop_();
}

//-----------------------------------------------------------------------------
// SPI initialization routine
// Parameter：Set lcd spi mode
//-----------------------------------------------------------------------------
void SPI_Init()	                      //Initialize SPI 
{
		_RD = 1;
		_UB = 1;
		_LB = 1;
		A0  = 0;
		D2  = 0;
		D3  = 0;
		D4  = 0;
		D5  = 0;
		D6  = 0;
		D7  = 0;
		LCDBUS_H = 0x00;
	  SPI_EN = 1;
	 	SO  = 1;
	  SI  = 1;
	}
//-----------------------------------------------------------------------------
// Write 16bit Data  routine
// Parameter：Write Data
// 1. Write one word 
//-----------------------------------------------------------------------------

	void SPI_WriteByte(uint writeByte)	//SPI写2个字节
{
		
	   uint i;
	  
		for(i=0;i<16;i++)
		{
			 			 
				SCLK      = 0;
				SI        = writeByte&0x8000; //si传输一位
				SCLK      = 1;
				writeByte = writeByte<<1;      
		}
		SCLK = 0;                         //SPI mode0
		
}
//-----------------------------------------------------------------------------
// Write 16bit Data  routine
// Parameter：Write Data
// 1. Write one word 
//-----------------------------------------------------------------------------
void SdData(uint DData)	              //SPI写2个字节
{
		uint i;
		for(i=0;i<16;i++)
		{
				SCLK  = 0;
				SI    = DData&0x8000;
				SCLK  = 1;
				DData = DData<<1;
		}
	  SCLK = 0;
		_CS =  1;
}
//-----------------------------------------------------------------------------
// Write 32bit Data Command routine
// Parameter：Write Data
// 1. Write the 16bit Command
// 2. Write the 16bit address
//-----------------------------------------------------------------------------
void SdCmd(ulong addr)
{
		 uchar       addr19;
     uint       command;
     addr19  = addr>>16;
	   command = 0x8800+addr19;
	    
	   _CS  = 1;
	   _CS  = 0;                       
		 SCLK = 0;
	  	
		SPI_WriteByte(0x88FF&command);	//写命令
		SPI_WriteByte(addr);            //写地址
	  SCLK = 0;                       
}

//-----------------------------------------------------------------------------
// Fill full screen routine
// Parameter：fill color
// 1. Write the start Address
// 2. Write the Horizontal Data
// 3. Change the next Horizontal Address
// 4. Write next Horizontal Data
//-----------------------------------------------------------------------------
void FillFullScn(uint color)
{
	ulong  addr;
	uint   i,j;
	addr=0;
	for(i=0;i<272;i++)  
	{
		SdCmd(addr);              // Write the start Address
		for(j=0;j<480;j++)
		{
			
			SdData(color);          // Write the Horizontal Data
			SdCmd(addr=addr+2);     // write next Address
		} 
		//addr=addr+960;  		  	//Next Horizontal Address
	}     
}
//

//----------------------------------
//   触摸屏驱动程序 TOUCH PANEL PROGRAAM
//----------------------------------

//----触摸屏驱动函数----------------
uint TdData(uchar Command)   //  Command为控制字
{ 
  uchar i;
  uint Tdata=0;
  TDCLK=0;                   // DCLK=0
  delayms(10);               // 延迟10ms，躲避触摸过程的抖动阶段（可调）
  transdata=Command;
  TDOUT=1; 
  TBUSY=1;
  T_CS=0;                    // 选通触摸屏
  for(i=0;i<8;i++)           // DCLK第1-8脉冲写控制字
	{
		TDIN=transbit7;          // 1位控制字数据给DIN
		TDCLK=1;                 // DCLK=1
		TDCLK=0;                 // DCLK=0
		transdata=transdata<<1;
  }

  delayms(2);    
	for(i=0;i<8;i++)           // DCLK第9-16脉冲读AD数据D11-D4位
	{
		transdata=transdata<<1;
		TDOUT=1;   
		TDCLK=1;                    // DCLK=1
		TDCLK=0;                    // DCLK=0
		transbit0=TDOUT;            // 延迟200ns以上读AD值transbit0
	}
  Tdata=transdata;              // 将D11-D4（8位数据）转移到中间变量
  Tdata=Tdata<<4;
  transdata=0;                  //准备下一轮的读取数据

  for(i=0;i<8;i++)              // 第17-24脉冲读AD数据D3-D0位和4个空操作
  {
     transdata=transdata<<1;
     TDOUT=1;      
     TDCLK=1;                   // DCLK=1
     TDCLK=0;                   // DCLK=0           
     transbit0=TDOUT;           // 延迟200ns以上读AD值
  }
  Tdata=Tdata|(transdata>>4);   // 组合成12位AD数据
  T_CS=1;                       // 封锁触摸屏
  return(Tdata);                // 返回给调用函数
}

//-------读取触摸屏点数值------------
void RdTData ()
{
		uint txdata,tydata;
		double  decimal_x,decimal_y;

		txdata=TdData(0xd0);        // 读取AD数据x值
		tydata=TdData(0x90);        // 读取AD数据y值	 
		decimal_x = ((double)(txdata-165 ) / (double)3755);
		decimal_y = ((double)(tydata-290 ) / (double)3534);   
		TP_x = (decimal_x * 472) ;
		TP_y = (decimal_y * 264);


}	 
//-----------------------------------------------------------------------------
// Fill mono Box routine
// Parameter：Box Width, start position of X,start position of Y，
//            fill front color, fill Background color, Color Data
// 1. Write the start Address
// 2. Write the Horizontal Data of front color or Background Color
// 3. Change the next Horizontal Address
// 4. Write next Horizontal Data
// PS:W=Width/8
//-----------------------------------------------------------------------------
void monoFill(uint H,uint W,uint x, uint y, fg_color, bg_color, uchar *Data )
{
		uchar i,j,k,Ddat;
		ulong addr;

		addr=960;//640
		addr=addr*y;
		addr=addr+x*2;

		for(i=0;i<H;i++)                     
		{
				SdCmd(addr);                       // Write the start Address
				for(j=0;j<W;j++)
				{
						Ddat=*Data++;  
						for(k=0;k<8;k++)
						{
								if((Ddat&0x80)==0x80)     // If the Hightest bit7(D7)=1
										SdData(fg_color);     // Write the front Color
								else                      // If the Hightest bit7(D7)=1
										SdData(bg_color);     // Write the Background Color
								Ddat=Ddat<<1;             // Color Data shift left 1bit, Write the next pixel Data
						}
				}
				addr=addr+960;  									// Next Horizontal Address  640
		}
}

//-----------------------------------
// wait...
//-----------------------------------
void WaitKey()
{ 
		bit WaitFlag;
		uint temp_x,temp_y;
		uint i,j;

		WaitFlag=1;
		delayms(250);
		while(WaitFlag)
		{
				temp_x=0;  
				temp_y=0;
				TP_x = 0;	
				TP_y = 0;
				if(TPENIRQ==0)                   	// PENIRQ Pressed, toggle the AutoRun flag
				{	 
			     
						RdTData() ;
						temp_x=TP_x;  
						temp_y=TP_y;
						RdTData() ;
						i=temp_x - TP_x;   
						if( i<0)  
							i=i*(-1); 
						j=temp_y - TP_y;	
						if( j<0)  
							j=j*(-1); 
						if((i<5)&&(j<5))
				 	  {	 
								TP_x = temp_x;	
								TP_y = temp_y;
				        monoFill(8, 1, TP_x, TP_y, GRAY04, WHITE, TDisplay); 
								if(TP_x>400 && TP_x<470) 						 
						     {	 	  
							      if(TP_y>200 && TP_y<265)	  					      
							      { 
												AutoRun=0; 
												WaitFlag=0; 
										}
						     }
					  }
				}
				if (AutoRun==1)             // Auto Run mode, do delay and quit
				{
						delayms(100);			    
						WaitFlag=0;
				}                        
		}
}


//-----------------------------------------------------------------------------
// LCD initialization routine
// 1. Set the Power mode to PSM0
// 2. PLL setting
// 3. Panel setting
// 4. main Panel RAM setting
// 5. PIP RAM setting
// 6. Set the Power mode to PSM1
//-----------------------------------------------------------------------------
void initLCM(void)
{
// PLL setting
	
   SdCmd(0x60804); SdData(0x0000);   // PSM0 Mode, Disable Memory and LCD Power / Clock
   delayms(100);
   SdCmd(0x60810); SdData(0x0000);   // Select PLL, Disable PLL
   SdCmd(0x60812); SdData(0x0003);   // N = 0, MM = 5 => CLKI = 10 MHz / 5 => 2MHz
   SdCmd(0x60814); SdData(0x0018);   // L = 0x18 => POCLK = 2MHz * 27 = 54 MHz
   SdCmd(0x60810); SdData(0x0001);   // Don't Enable PLL, Bypass 
   delayms(100);
   SdCmd(0x60816); SdData(0x0006);   // With PLL: PCLK := 54MHz / 6 = 9MHz   
   SdCmd(0x60804); SdData(0x0001);   // Enable Memory Power / Clock
   delayms(100);
// Panel setting
   SdCmd(0x60820); SdData(0x008F);   // DE is low, Data ready on PCLK rising edge, 24bit enabled
   SdCmd(0x60822); SdData(0x0001);   // TE is off, Inverted Data
   SdCmd(0x60824); SdData(0x003C);   // The display period of HS = 480/8 = 60
   SdCmd(0x60826); SdData(0x002D);   // The empty period of HS = 525-480=45
   SdCmd(0x6082c); SdData(0x0002);   // HS Polarity = 0, Pulse width = 2
   SdCmd(0x6082e); SdData(0x0002);   // The start position of HS = 2
   SdCmd(0x60828); SdData(0x0110);   // The display period of VS = 272
   SdCmd(0x6082a); SdData(0x000D);   // The empty period of VS = 285-272=13  //old TFT is set to 0x000E, new TFT is set to 0x000D
   SdCmd(0x60830); SdData(0x0002);   // VS Polarity = 0, Pulse width = 2
   SdCmd(0x60832); SdData(0x0002);   // The start position of VS = 2  
// Main Panel RAM setting
   SdCmd(0x60840); SdData(0x0001);   // Main Panel&PIP enable, and Main Panel display data is RGB=5:6:5
   SdCmd(0x60842); SdData(0x0000);   // The start Address of Main Panel = 00000h
   SdCmd(0x60844); SdData(0x0000);   //
// PIP RAM setting
   SdCmd(0x60850); SdData(0x0001);   //  PIP display data is RGB=565
   SdCmd(0x60852); SdData(0xFF70);   //  The start Address of PIP Panel = 3FF70h
   SdCmd(0x60854); SdData(0x0003);   //

   SdCmd(0x60856); SdData(0x0064);   //  Display width of PIP = 100
	 SdCmd(0x60858); SdData(0x0039);   //  Display width of PIP = 57
   SdCmd(0x6085a); SdData(0x0000);   //  The start position of X of PIP = 0
   SdCmd(0x6085c); SdData(0x0000);   //  The start position of Y of PIP = 0
	 
   SdCmd(0x60860); SdData(0x0000);   //  No PIP
	 SdCmd(0x60862); SdData(0x0000);   //   
	 SdCmd(0x60864); SdData(0x0000);   //  Transparency disable 
	 SdCmd(0x60866); SdData(0x0000);   //  Transparency Key Color setting 
	 SdCmd(0x60868); SdData(0x0000);   //  Transparency Key Color setting
	 
	 //GPIO setting	 
	 SdCmd(0x608D0); SdData(0x0007);   //  GPIO3 is input, GPIO[2:0] are output	
	 SdCmd(0x608D2); SdData(0x0007);   //  GPIO[2:0] output high,TFT display normal mode

   SdCmd(0x60804); SdData(0x0002);   //  PSM1 Mode, Enable Mem and Panel Clock
   delayms(100);   
}
//

//-----------------------------------------------------------------------------
// Main program
// 1. I/O initialization
// 2. Reset MCU
// 3. LCD initialization
// 4. Loop Fill Picture
//-----------------------------------------------------------------------------
void main()
{

	  EA      = 0;               // no interrupt
	  SCLK    = 0;

		BL_ADJ  = 0;           		// Close the Backlight of TFT
    _TP_CS    = 1;          		// TP.CS
    TP_DCLK   = 0;
    TP_DIN    = 0;
    TP_DOUT   = 1;
    _TP_PENQ  = 1;          		// 0 = touched
	  SPI_EN  = 1;
	  SPI_Init();
	
		AutoRun = 0;
		_RST=1; delayms(100);     // wait for all power stable
		_RST=0; delayms(100);     // reset pulse
		_RST=1; delayms(500);     // wait till internal reset routine finish
	  SPI_EN = 1;              // 使能SPI
	  SO  = 1;              
	  SI  = 1;
	  SCLK = 0;
	  _CS = 0;
					
		initLCM();              // LCD initialization
	  BL_ADJ = 1;              // Open the Backlight of TFT	
	  delayms(100);
		
  
		while(1)
		{				
				//_CS = 0;	
				FillFullScn(RED); delayms(100); WaitKey(); delayms(500);//全红
				FillFullScn(GREEN); delayms(100); WaitKey(); delayms(500);//全绿
				FillFullScn(BLUE); delayms(100); WaitKey(); delayms(500);//全蓝
				FillFullScn(WHITE);delayms(100); WaitKey(); delayms(500);//全白
				FillFullScn(BLACK);delayms(100); WaitKey();delayms(500);//全黑
					 
		}
	 
}

//

//-----------------------------------------------------------------------------
// End off
//-----------------------------------------------------------------------------

