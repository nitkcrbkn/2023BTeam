#include "app.h"
#include "DD_Gene.h"
#include "DD_RCDefinition.h"
#include "SystemTaskManager.h"
#include <stdlib.h>
#include "message.h"
#include "MW_GPIO.h"
#include "MW_IWDG.h"
#include "MW_flash.h"
#include "constManager.h"
#include "trapezoid_ctrl.h"
/*suspensionSystem*/


static
int suspensionSystem(void);

static
int OtasukeUpDownSystem(void);

static
int forwardWheelLeft(void);

static
int forwardWheelRight(void);

/* 腕振り部の変数 */
int situation = 0;
int judgepush = 0;

/* モードチェンジ用変数 */
int mode = 0;
int checkpush = 0;

/*メモ
 *g_ab_h...ABのハンドラ
 *g_md_h...MDのハンドラ
 *
 *g_rc_data...RCのデータ
 */

#define WRITE_ADDR (const void*)(0x8000000+0x400*(128-1))/*128[KiB]*/
flashError_t checkFlashWrite(void){
    const char data[]="HelloWorld!!TestDatas!!!\n"
        "however you like this microcomputer, you don`t be kind to this computer.";
    return MW_flashWrite(data,WRITE_ADDR,sizeof(data));
}

int appInit(void){
    message("msg","hell");

    /* switch(checkFlashWrite()){ */
    /* case MW_FLASH_OK: */
    /*     message("msg","FLASH WRITE TEST SUCCESS\n%s",(const char*)WRITE_ADDR); */
    /*     break; */
    /* case MW_FLASH_LOCK_FAILURE: */
    /*     message("err","FLASH WRITE TEST LOCK FAILURE\n"); */
    /*     break; */
    /* case MW_FLASH_UNLOCK_FAILURE: */
    /*     message("err","FLASH WRITE TEST UNLOCK FAILURE\n"); */
    /*     break; */
    /* case MW_FLASH_ERASE_VERIFY_FAILURE: */
    /*     message("err","FLASH ERASE VERIFY FAILURE\n"); */
    /*     break; */
    /* case MW_FLASH_ERASE_FAILURE: */
    /*     message("err","FLASH ERASE FAILURE\n"); */
    /*     break; */
    /* case MW_FLASH_WRITE_VERIFY_FAILURE: */
    /*     message("err","FLASH WRITE TEST VERIFY FAILURE\n"); */
    /*     break; */
    /* case MW_FLASH_WRITE_FAILURE: */
    /*     message("err","FLASH WRITE TEST FAILURE\n"); */
    /*     break;                 */
    /* default: */
    /*     message("err","FLASH WRITE TEST UNKNOWN FAILURE\n"); */
    /*     break; */
    /* } */
    /* flush(); */

    ad_init();

    message("msg","plz confirm\n%d\n",g_adjust.rightadjust.value);

    /*GPIO の設定などでMW,GPIOではHALを叩く*/
    return EXIT_SUCCESS;
}

/*application tasks*/
int appTask(void){
    int ret=0;

    /*if(__RC_ISPRESSED_R1(g_rc_data)&&__RC_ISPRESSED_R2(g_rc_data)&&
         __RC_ISPRESSED_L1(g_rc_data)&&__RC_ISPRESSED_L2(g_rc_data)){
        while(__RC_ISPRESSED_R1(g_rc_data)||__RC_ISPRESSED_R2(g_rc_data)||
	    __RC_ISPRESSED_L1(g_rc_data)||__RC_ISPRESSED_L2(g_rc_data));
        ad_main();
        }*/
	 
    /*それぞれの機構ごとに処理をする*/
    /*途中必ず定数回で終了すること。*/
    ret = suspensionSystem();
    if(ret){
        return ret;
    }

  ret = forwardWheelLeft();
  if(ret){
      return ret;
  }
  ret = OtasukeUpDownSystem();
  if(ret){
    return ret;
  }

    ret = forwardWheelRight();
    if(ret){
        return ret;
    }
	 
    return EXIT_SUCCESS;
}


/*プライベート 足回りシステム*/
static
int suspensionSystem(void){
    const int num_of_motor = 2;/*モータの個数*/
    int rc_analogdata;/*アナログデータ*/
    unsigned int idx;/*インデックス*/
    int i;

    const tc_const_t tc ={
        .inc_con = 100,
        .dec_con = 225,
    };

    /* コントローラのボタンは押されてるか */
    if(!__RC_ISPRESSED_TRIANGLE(g_rc_data)){
        checkpush = 1;
    }
    

    if(__RC_ISPRESSED_TRIANGLE(g_rc_data) && checkpush == 1 && !__RC_ISPRESSED_CROSS(g_rc_data)){
	        
        switch(mode){
        case 1:
            mode = 0;
            break;
        case 0:
            mode = 1;
            break;
        }
	        
        checkpush = 0;
    }

    switch(mode){
    case 0:
        /*for each motor*/
        for(i=0;i<num_of_motor;i++){
            /*それぞれの差分*/
            switch(i){
                case 0:
	                idx = MECHA1_MD0;
	                rc_analogdata = DD_RCGetRY(g_rc_data);
	                break;

                case 1:
	                idx = MECHA1_MD1;
	                rc_analogdata = -DD_RCGetLY(g_rc_data);
                    break;
                
                default:
                    return EXIT_FAILURE;
            }

            int dutyX;

            if(__RC_ISPRESSED_L1(g_rc_data) && __RC_ISPRESSED_R1(g_rc_data)){
                dutyX = 4;
            }
            else if(__RC_ISPRESSED_L1(g_rc_data) || __RC_ISPRESSED_R1(g_rc_data)){
                dutyX = 2;
            }
            else{
                dutyX = 1;
            }

            for(int j=0;j<=2;j+=2){ //2つのタイヤを回転させるためにfor文
                if(j==2 && i==0 && __RC_ISPRESSED_R1(g_rc_data) && DD_RCGetRY(g_rc_data)==0){
                    break;
                }
                else if(j==2  && i==1 && __RC_ISPRESSED_L1(g_rc_data) && DD_RCGetLY(g_rc_data)==0){
                    break;
                }
                if(DD_RCGetRY(g_rc_data)-DD_RCGetLY(g_rc_data) >= -10 && DD_RCGetRY(g_rc_data)-DD_RCGetLY(g_rc_data) <= 10){
                    trapezoidCtrl(rc_analogdata * MD_GAIN_MAX / dutyX,&g_md_h[idx+j],&tc);
                }
                else{
                    trapezoidCtrl(rc_analogdata * MD_GAIN_NORMAL / dutyX,&g_md_h[idx+j],&tc);
                }
            }
        }
        break;

    /*case 1:
        for(i=0;i<num_of_motor;i++){
            switch(i){
            case 0:
	idx = MECHA1_MD1;
	rc_analogdata = DD_RCGetRY(g_rc_data);
	break;
            case 1:
	idx = MECHA1_MD0;
	rc_analogdata = -DD_RCGetLY(g_rc_data);
	break;         
	        
            default:
	return EXIT_FAILURE;
            }
            trapezoidCtrl(rc_analogdata * MD_GAIN,&g_md_h[idx],&tc);
        }
        break;*/
    }
    return EXIT_SUCCESS;
}

static
int forwardWheelLeft(void){
    unsigned int idx=3;/*インデックス*/
    const tc_const_t tc ={
            .inc_con = 100,
            .dec_con = 225,
    };
    int duty=0;

    /*走行中前輪単独駆動はさせない*/
    if(DD_RCGetLY(g_rc_data)==0){
        if(__RC_ISPRESSED_L2(g_rc_data)){
            duty = 1000;
        }
        else{
            duty = 0;
        }
        trapezoidCtrl(duty,&g_md_h[idx],&tc);
    }

    return EXIT_SUCCESS;
}

static
int OtasukeUpDownSystem(void){
  unsigned int idx = 2;
  int i;
  int duty = 0;

  const tc_const_t tc ={
    .inc_con = 100,
    .dec_con = 225,
  };
  if(__RC_ISPRESSED_UP(g_rc_data)){
      duty = -6000;
  } else if(__RC_ISPRESSED_DOWN(g_rc_data)){
      duty = 6000;
  } else{
      duty = 0;
  }

  trapezoidCtrl(duty,&g_md_h[idx],&tc);

  return EXIT_SUCCESS;
}

static
int forwardWheelRight(void){
    unsigned int idx=2;/*インデックス*/
    const tc_const_t tc ={
            .inc_con = 100,
            .dec_con = 225,
    };
    int duty=0;

    /*走行中前輪単独駆動はさせない*/
    if(DD_RCGetRY(g_rc_data)==0){
        if(__RC_ISPRESSED_R2(g_rc_data)){
            duty = -1000;
        }
        else{
            duty = 0;
        }
        trapezoidCtrl(duty,&g_md_h[idx],&tc);
    }

    return EXIT_SUCCESS;
}