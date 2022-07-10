/*
 *      LED_Strobe_v2
 *          with Start Trigger
 *          with Muzzle Velocity 
 * 
 *          Fosc 64MHz (Xtal:16MHz x4PLL)
 * 
 *  2022.06.11  ver.0.01    PWMでLEDを点灯
 *  2022.06.18  ver.0.10    トリガーインプット追加
 *                          98m/s対応 10kHz duty10%  周期100usec パルス10usec   (v0=100m/sで10mm毎)
 *                          PWM1PR:0x18ff = 6399, PWM1S1P1:0x0280 = 640
 *  2022.06.20  ver.0.11    43m/s対応 5kHz duty10%  周期200usec パルス10usec    (v0=50m/sで10mm毎)
 *                          PWM1PR:12799, PWM1S1P1:0x0280 = 640
 *  2022.06.20  ver.0.12    26m/s対応　2.5kHz duty10%  周期400usec パルス10usec　(v0=25m/sで10mm毎)
 *                          PWM1PR:25599, PWM1S1P1:0x0280 = 640
 *  2022.06.20  ver.0.13    34m/s対応　3.3kHz duty10%  周期300usec パルス10usec　(v0=33m/sで10mm毎)
 *                          PWM1PR:19199, PWM1S1P1:0x0280 = 640
 *  2022.06.24  ver.0.14    DEBUGger表示のタイミングを撮影に適するように変更。
 *                          発光周期を初速で計算し、12mm間隔に写るように。
 *  2022.06.25  ver.0.20    MCCのトラブルのため_v2にコピー
 *  2022.06.25  ver.0.21    点滅発光時間10msec->12msec VSRの時明るくなりすぎる
 *  2022.06.27  ver.0.22    発光パルス10us→5us。発光回数の制限。
 * 
 * 
 */

#include "mcc_generated_files/system/system.h"
#include "header.h"

/*
    Main application
*/
bool        timer1_int_flag = 0;
bool        cmp1_int_flag = 0;
bool        cmp2_int_flag = 0;
uint32_t    t;
uint16_t    t_light;
double      v0, pe;

void cmp1_int(void){
    cmp1_int_flag = 1;
}

void cmp2_int(void){
    cmp2_int_flag = 1;
}


int main(void)
{
    SYSTEM_Initialize();    
    
#if (PWM_10KHZ == true)
    //PWM1_16BIT_WritePeriodRegister(6399);     //10kHz 100m/s
    //PWM1_16BIT_WritePeriodRegister(12799);    //5kHz   50m/s
    //PWM1_16BIT_WritePeriodRegister(25599);    //2.5kHz 25m/s
    PWM1_16BIT_WritePeriodRegister(19199);      //3.3kHz 30m/s
#endif
    //PWM pulse width
    //PWM1_16BIT_SetSlice1Output1DutyCycleRegister(640);    //10us  1/100000sec
    //PWM1_16BIT_SetSlice1Output1DutyCycleRegister(480);    //7.5us 1/133333sec
    PWM1_16BIT_SetSlice1Output1DutyCycleRegister(320);    //5us   1/200000sec

    
    // If using interrupts in PIC18 High/Low Priority Mode you need to enable the Global High and Low Interrupts 
    // If using interrupts in PIC Mid-Range Compatibility Mode you need to enable the Global Interrupts 
    // Use the following macros to: 

    // Enable the Global Interrupts 
    INTERRUPT_GlobalInterruptEnable(); 

    // Disable the Global Interrupts 
    //INTERRUPT_GlobalInterruptDisable();
    
    __delay_ms(1000);
    LED_YELLOW_SetLow();
    LED_WHITE_SetLow();
    printf("\n");
    printf("********************\n");
    printf(" LED Strobe v2      \n");
    printf("  w/ trigger        \n");
    printf("            ver.0.22\n");
    printf("********************\n");
    printf("\n");
    __delay_ms(500);
    LED_BLUE_SetLow();

    cmp1_int_flag = 0;
    cmp2_int_flag = 0;
    PWM1_16BIT_Disable();
    SMT1_DataAcquisitionEnable();
    
    while(1)
    {
        if (cmp2_int_flag == 1){
            //auto period
            t = SMT1_GetCapturedPeriod();                                   //撮影間隔を12mm間隔にしてみる
            PWM1_16BIT_WritePeriodRegister((uint16_t)(t & 0x0000ffff));     //cmp2がオンしてから
            
            PWM1_16BIT_Enable();
            //発光時間タイマーの計算
            t_light = 0xffff - (uint16_t)(t & 0x0000ffff);                  //TMR1 FOSC/4 pre1:8 なので32パルス分のカウント値になる
            Timer1_Write(t_light);
            Timer1_Start();
            //__delay_ms(5);         //ストロボ撮影時間
            while(!Timer1_HasOverflowOccured()){
            }
            PWM1_16BIT_Disable();
            //Clear interrupt flags
            PIR3bits.TMR1IF = 0;
            Timer1_Stop();
            
            //
            //LED_YELLOW_SetHigh();
            //printf("sensor1 on\n");
            //LED_YELLOW_SetLow();
            
            //初速の計算
            //t = SMT1_GetCapturedPeriod();
            //v0 = 0.0120 / t * _XTAL_FREQ;            
            pe = (double)t / _XTAL_FREQ;
            v0 = 0.0120 / pe;
            if ((v0 < 200) && (v0 > 0)){
                printf("%5.1fm/s (%5.1fus)", v0, pe * 1000000);
            }else {
                printf("v0:error");
            }
            
            //SMT1再セット
            SMT1_DataAcquisitionDisable();
            SMT1_DataAcquisitionEnable();
            
            //カメラのシャッター閉じ待ち
            __delay_ms(1000);
            printf("\n");       //撮影中のスクロールを防ぐ
            
            //
            cmp1_int_flag = 0;
            cmp2_int_flag = 0;
        }
        
    }    
}


