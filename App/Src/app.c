#include "app.h"
#include "DD_Gene.h"
#include "DD_RCDefinition.h"
#include "SystemTaskManager.h"
#include <stdlib.h>
#include "MW_GPIO.h"
#include "MW_IWDG.h"
#include "message.h"
#include "MW_flash.h"
#include "constManager.h"
#include <trapezoid_ctrl.h>
#include "stm32f1xx_hal.h"
//#include "mbed.h"

static
int suspensionSystem(int inx, int iny); //足回り


static
int cont(void);
//static int LEDSystem(int redledmode,int blueledmode,int greanledmode);


/*メモ
 *g_ab_h...ABのハンドラ
 *g_md_h...MDのハンドラ
 *
 *g_rc_data...RCのデータ
 */

int mode=0;
int flush_ok=1;

int appInit(void){

  ad_init();

  /*GPIO の設定などでMW,GPIOではHALを叩く*/
  return EXIT_SUCCESS;
}

/*application tasks*/
int appTask(void){
  int ret=0;
  /*それぞれの機構ごとに処理をする*/
  /*途中必ず定数回で終了すること。*/
  
  ret = cont();
  if(ret){
    return ret;
  }

    return EXIT_SUCCESS;
}

static int cont(void){

  //static int (*towfcp[])(int) = {ThrowSystem_1,ThrowSystem_2,ThrowSystem_3,ThrowSystem_4};
  //static int (*shefcp[])(int) = {ThrowSystem_5};//関数ポインタ定義、使う関数の指定を関数ポインタで　まだ未完成
  int ret;
  //int i = 0;
  static int screencheck1=0,screencheck2=0,screencheck3=0;
  //static int reloadcheck1=0,reloadcheck2=0,reloadcheck3=0;
  static int towelmode=-1,sheetmode=-1;
  //static int towelpress=0,towelcheck=0,towelgo=0;
  //static int sheetpress=0,sheetcheck=0,sheetgo=0;
  static int beformode=0;
  //static int towelstatus=1;
  //static int sheetstatus=1;
  //  static int color1=0,color2=0,color3=0,color4=0;
  static int count=0;
  static int inx=0;
  static int iny=0;
  static int redledmode=0,blueledmode=0,greanledmode=0;
  //画面
  if((__RC_ISPRESSED_R2(g_rc_data))&&(__RC_ISPRESSED_L2(g_rc_data))){
    if(__RC_ISPRESSED_UP(g_rc_data))screencheck1=1;
    if(__RC_ISPRESSED_RIGHT(g_rc_data)&&screencheck1)screencheck2=1;
    if(__RC_ISPRESSED_DOWN(g_rc_data)&&screencheck2)screencheck3=1;
    if(__RC_ISPRESSED_LEFT(g_rc_data)&&screencheck3){
      mode^=1;screencheck1=0;screencheck2=0;screencheck3=0;
    }
  }else{
    screencheck1=0;screencheck2=0;screencheck3=0;
  }

  if(__RC_ISPRESSED_R1(g_rc_data)&&__RC_ISPRESSED_L1(g_rc_data)){
    redledmode=1;
  }else{
    redledmode=0;
  }

  /*ret = LEDSystem(redledmode,blueledmode,greanledmode);
  if(ret){
    return ret;
  }*/

  ret = suspensionSystem(inx,iny);
  if(ret){
    return ret;
  }

  

  
  count++;
  if(flush_ok){
    if(!mode){
      MW_printf("\ntmode[%2d] smode[%2d] NUM_MD[%d] RLED[%d] BLED[%d] GLED[%d]",towelmode,sheetmode,DD_NUM_OF_MD,redledmode,blueledmode,greanledmode);
    }else{
      MW_printf(/*左スティックで移動、右スティックで旋回\n丸でタオル砲、三角でシーツ砲\nR1、L1同時押ししながら↑ → ↓ ← の順に押すと各砲をリセット\n*/"MD\n0番：左手前\n1番：左奥\n2番：右奥\n3番：右手前");
    }
    flush_ok=0;
  }

  if(beformode!=mode){
    MW_printf("\33[2J");
    beformode=mode;
  }
  return EXIT_SUCCESS;
}

static
int suspensionSystem(int inx,int iny){
  const int num_of_motor = 4;/*モータの個数*/
  //  int rc_analogdata;/*アナログデータ*/
  unsigned int idx=0;/*インデックス*/
  int i=0,x=0,y=0,w=0;
  int m=0;
  int adjust;
  // static int x_ok=0,y_ok=0;
  //static int xflag=0,yflag=0;
 const tc_const_t tc ={
    .inc_con = 400,
    .dec_con = 450,
 };

 
 if(iny){
   x = iny;
 }else{
   x = DD_RCGetLY(g_rc_data);
 }

 if(inx){
   y = inx;
 }else{
    y = DD_RCGetLX(g_rc_data);
 }

 /*
 if(iny){
   
   if(iny>0||yflag){
     if(x<0&&i<10){
       yflag=1;
       i++;
     }else{
       x_ok=1;
     }
   }else if(iny<0||yflag){
     if(x>0&&i<10){
       yflag=1;
       i++;
     }else{
       x_ok=1;
     }
   }else{
     x_ok=1;
   }
   if(x_ok){
     x = iny;
     i=0;
     yflag=0;
   }
 }else{
   
   if(DD_RCGetLY(g_rc_data)>0||yflag){
     if(x<0&&i<10){
       yflag=1;
       i++;
     }else{
       x_ok=1;
       i=0;
     }
   }
   if(DD_RCGetLY(g_rc_data)<0){
     if(x>0&&i<10){
       i++;
     }else{
       x_ok=1;
       i=0;
     }
   }
   if(x_ok){
     x = DD_RCGetLY(g_rc_data);
   }
 }

  if(inx){

   if(inx>0){
     if(y<0&&i<10){
       i++;
     }else{
       y_ok=1;
       i=0;
     }
   }else if(inx<0){
     if(y>0&&i<10){
       i++;
     }else{
       y_ok=1;
       i=0;
     }
   }
   if(y_ok){
     y = inx;
   }
 }else{
   if(DD_RCGetLX(g_rc_data)>0){
     if(y<0&&i<10){
       i++;
     }else{
       y_ok=1;
       i=0;
     }
   }
   if(DD_RCGetLX(g_rc_data)<0){
     if(y>0&&i<10){
       i++;
     }else{
       y_ok=1;
       i=0;
     }
   }
   if(y_ok){
     y = DD_RCGetLX(g_rc_data);
   }
 }
 */
 
 w = -(int)(DD_RCGetRX(g_rc_data)*WISE_MAG);
 /*
 if(x>0){
   w - 9;
 }
 */ /* if(__RC_ISPRESSED_R2(g_rc_data)){
     w = 60;
     }else if(__RC_ISPRESSED_L2(g_rc_data)){
     w = -60;
     } else {
     w = 0;
     }*/


  /*for each motor*/
  for(i=0;i<num_of_motor;i++){
    /*それぞれの差分*/
    switch(i){//トランザム
    case 0:
      idx = MECHA1_MD0; //駆動左前用モータ
      m =  x + y + w;
      m *= (int)(75*MD0MAG);
      if(abs(m) <= 3200) {//dutyが低かったら引き上げ
	m *= 2;
      } else if(abs(m) >= 6500) {//dutyが6500を超えたら6500以下になるよう調整
	adjust = abs(m) - 6500;
	if(m > 0) {
	  m -= adjust;
	} else if(m < 0) {
	  m += adjust;
	}
      }
      break;
      
    case 1:
      idx = MECHA1_MD1; //駆動左後用モータ
      m =  x - y + w;
      m *= 75;
      if(abs(m) <= 3200) {//dutyが低かったら引き上げ
	m *= 2;
      } else if(abs(m) >= 6500) {//dutyが6500を超えたら6500以下になるよう調整
	adjust = abs(m) - 6500;
	if(m > 0) {
	  m -= adjust;
	} else if(m < 0) {
	  m += adjust;
	}
      }
      break;
      
    case 2:
      idx = MECHA1_MD2; //駆動右後用モータ
      m = - x - y + w;
      m *= (int)(75*MD2MAG);
      if(abs(m) <= 3200) {//dutyが低かったら引き上げ
	m *= 2;
      } else if(abs(m) >= 6500) {//dutyが6500を超えたら6500以下になるよう調整
	adjust = abs(m) - 6500;
	if(m > 0) {
	  m -= adjust;
	} else if(m < 0) {
	  m += adjust;
	}
      }
      break;
      
    case 3:
      idx = MECHA1_MD3; //駆動右前用モータ
      m = - x + y + w;
      m *= 75;
      if(abs(m) <= 3200) {//dutyが低かったら引き上げ
	m *= 2;
      } else if(abs(m) >= 6500) {//dutyが6500を超えたら6500以下になるよう調整
	adjust = abs(m) - 6500;
	if(m > 0) {
	  m -= adjust;
	} else if(m < 0) {
	  m += adjust;
	}
      }
      break;
      
      return EXIT_FAILURE;
    }
    trapezoidCtrl((int)(SUSPENSION_MAG*m),&g_md_h[idx],&tc);
  }

  return EXIT_SUCCESS;
}


/*static int LEDSystem(int redledmode,int blueledmode,int greanledmode){
  switch(redledmode){
  case 0:
    MW_GPIOWrite(GPIOBID,GPIO_PIN_13, GPIO_PIN_RESET);
    break;
  case 1:
    MW_GPIOWrite(GPIOBID,GPIO_PIN_13, GPIO_PIN_SET);
    break;
  case 2:
    MW_GPIOWrite(GPIOBID,GPIO_PIN_13, GPIO_PIN_SET);
    break;
  }
  
  switch(blueledmode){
  case 0:
    MW_GPIOWrite(GPIOBID,GPIO_PIN_14, GPIO_PIN_RESET);
    break;
  case 1:
    MW_GPIOWrite(GPIOBID,GPIO_PIN_14, GPIO_PIN_SET);
    break;
  }

  switch(greanledmode){
  case 0:
    MW_GPIOWrite(GPIOBID,GPIO_PIN_15, GPIO_PIN_RESET);
    break;
  case 1:
    MW_GPIOWrite(GPIOBID,GPIO_PIN_15, GPIO_PIN_SET);
    break;
  }
  return EXIT_SUCCESS;
}
*/