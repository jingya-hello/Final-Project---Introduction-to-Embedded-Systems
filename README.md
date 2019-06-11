# CS 4101 Introduction to Embedded Systems Fall Semester 2017
## 104000033 邱靖雅
# Robot Cleaner

## Result
<a href="http://www.youtube.com/watch?feature=player_embedded&v=KeE3YuLyaLM
" target="_blank"><img src="http://img.youtube.com/vi/KeE3YuLyaLM/0.jpg" 
alt="Our video (fail to load the image, please click here to see the video)" width="240" height="180" border="10" /></a>

## Goal : 
保持環境整潔一直是一件困難的事，因此希望製作出簡易的自走車，並於自走車安裝衛生紙、夾取手臂掃地工具，建造出掃地機器人協助打掃環境。
## Functionality : 
1. 打掃環境： 轉動衛生紙擦拭地板，並隨著車子移動增加擦拭範圍。
2. 閃避障礙： 車子能夠自動閃避障礙，轉動前進方向。
3. 模式設定：有兩種操作模式分別是IRremote遙控及自走模式。
4. 夾取垃圾：利用手臂撿起地上物品
5. 唱歌：撥放背景音樂
6. 增加光線：當環境光不足，開啟車尾及車頭燈。

## Components

|LCD|顯示當前模式及車子行走情況|
|---|---|
|IRremote|開機、切換模式、移動車子|
|Arduino|Arduino!|
|L298P驅動模塊|驅動直流電機|
|Arduino擴展版|擴展Pin腳|
|直流馬達|驅動車子前進|
|光敏電阻|判斷環境光線決定是否開車燈|
|LED燈|車頭燈|
|RGB燈|車尾燈|
|超聲波感應器|偵測是否有障礙物|
|Buzzer|播放背景音樂|
|Servo1|使超聲波感測器轉動|
|Servo2|轉動擦地板的抹布|
|Servo3|操控手臂夾取物品|

## Implementations

1. 切換模式：

    |IR remote|0|100+|200+|
    |---|---|---|---|
    |State|Off mode→Auto mode<br>Auto or Control→Off mode|If(!Off mode)Auto mode|If(!Off mode)Control mode|
    
    * Off Mode：關機狀態，按0可切換至Auto Mode
    * Auto Mode：自走狀態，偵測障礙物自行移動
    * Control Mode：控制狀態，可使用IRremote控制前後左右

2. 直流馬達：
    * 左馬達/右馬達

        |Pin1|Pin2|動作|
        |---|---|---|
        |High|Low|馬達1前進|
        |Low|High|馬達1後退|
        |High|High|馬達1停止|
        |Low|Low|馬達1滑行|

    * 車子行駛
    
        |左馬達|右馬達|動作|
        |---|---|---|
        |前進|前進|車子前進|
        |後退|後退|車子後退|
        |前進|後退|車子右轉|
        |後退|前進|車子左轉|
        |前進|滑行|車子向右自轉|
        |停止|停止|車子煞車|
        
3. 車子自走及閃避障礙：
利用Ultrasonic module置於車前，偵測前方與障礙物的距離：
    * 當與前方障礙物距離小於20公分：
        * 煞車、後退、煞車
        * 轉動servo偵測與右方障礙物距離
        * 轉動servo偵測與左方障礙物距離
        * 當與右方障礙物距離與左方障礙物距離皆小於20公分：
            * 原地右轉
        * 當與右方障礙物距離 > 與左方障礙物距離：
            * 右轉
        * 當與右方障礙物距離 < 與左方障礙物距離：
            * 左轉
        * 前進
4.	IRremote控制：
當當前模式為control，根據IRremote收到2、4、6、8來更改車子移動方向。
5. 撿起物品：
![](https://i.imgur.com/bDJch03.png)
6.	擦地板
Servo上黏貼衛生紙，重複將servo從0度轉到180度再轉回0度。
7.	燈光偵測
當環境光大於指定值：LED→LOW
其他：LED→HIGH
8.	Buzzer
定義音名的frequency ，
根據樂譜設定撥放的音。
![](https://i.imgur.com/a2t2HNO.png)

## Difficulties : 
1.	上課是利用timer1的interrupt教學但servo的library已經使用timer1，需要改成其他timer：
參考網路上的不同timer的setup教學及timer0的HZ改為使用timer0實作timer interrupt ( http://www.instructables.com/id/Arduino-Timer-Interrupts/ ) 
2.	timer interrupt使用timer0實作、servo的library使用timer1、buzzer和IRremote的library皆須使用timer2實作，整體的timer數量不足：
將buzzer實作在第2張arduino，同時因為servo數量過多analog pin的數量不足，因此部分實作在第2張arduino。
3.	使用的車子本體為arduino + L298P驅動器 + Arduino v1擴展版，且使用KAISE公司製作的版本，但KAISE公司似乎倒閉了，找不到datasheet，因此擴展板需手動測試，找到預設的pin腳。例如：擴展版直接插在驅動器上，需一個一個測試驅動器使用的digital pin；擴展版有保留IR remote感應器的位置可以直接焊接到擴展版上，但同樣須測試找pin腳。

