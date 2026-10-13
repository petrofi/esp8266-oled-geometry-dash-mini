/*
  GEOMETRY DASH MINI — ESP8266 + SH1106 128x64 OLED
  Controls: phone touchscreen (browser over ESP8266's own Wi-Fi).
  No TTP223, button, router, Internet, or extra components required.

  Wiring (ESP8266 NodeMCU / Wemos D1 mini):
     OLED GND  -> GND
     OLED VCC  -> 3V3
     OLED SCL  -> D1 (GPIO5)
     OLED SDA  -> D2 (GPIO4)

  Arduino libraries:
     Adafruit SH110X
     Adafruit GFX Library
     Adafruit BusIO (dependency)
  Bundled with ESP8266 board package: Wire, ESP8266WiFi,
     ESP8266WebServer, EEPROM.

  ESP8266 AP: GeometryDash-ESP8266  /  geomdash123
  Open on phone: http://192.168.4.1
*/
#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

constexpr uint8_t OLED_SDA = 4;       // D2
constexpr uint8_t OLED_SCL = 5;       // D1
constexpr uint8_t OLED_ADDRESS = 0x3C; // Try 0x3D if needed
constexpr int16_t SCREEN_W = 128;
constexpr int16_t SCREEN_H = 64;
constexpr int16_t FLOOR_Y = 54;
constexpr int16_t PLAYER_X = 22;
constexpr int16_t PLAYER_SIZE = 9;
constexpr uint32_t FRAME_MS = 20;   // Physics target: 50 updates/sec
constexpr uint32_t DRAW_MS = 40;    // OLED target: up to 25 updates/sec
constexpr uint32_t HIGH_SCORE_MAGIC = 0x47444D32UL;

const char *AP_SSID = "GeometryDash-ESP8266";
const char *AP_PASSWORD = "geomdash123"; // At least 8 characters

Adafruit_SH1106G oled(SCREEN_W, SCREEN_H, &Wire, -1);
ESP8266WebServer server(80);

const char PAGE_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="tr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<meta name="theme-color" content="#0a1120">
<title>Geometry Dash Mini - ESP8266</title>
<style>
*{box-sizing:border-box}
html,body{margin:0;min-height:100%;font-family:system-ui,-apple-system,Arial,sans-serif;background:#0a1120;color:#f4f9ff;text-align:center}
body{user-select:none;-webkit-user-select:none}
main{max-width:540px;margin:auto;padding:20px 17px 26px}
h1{font-size:26px;letter-spacing:1.2px;margin:12px 0 3px}
p{color:#aebed2;font-size:13px;line-height:1.5}
.box{background:#14243b;border:1px solid #354661;border-radius:22px;padding:19px 15px;margin-top:20px}
.stats{display:grid;grid-template-columns:repeat(3,1fr);gap:7px;margin:15px 0 18px}
.stat{background:#1b314c;border-radius:10px;padding:10px 5px;font-size:12px;color:#bed2e6}
.stat b{display:block;color:white;font-size:24px}
.display{border:3px solid #62738c;border-radius:10px;background:#070c14;max-width:320px;margin:0 auto 18px;padding:18px 8px;color:#b0f7e2;font:13px monospace}
.art{position:relative;height:61px;border-bottom:2px solid #b0f7e2;margin:9px auto 0;max-width:245px;text-align:left}
.cube{position:absolute;bottom:0;left:36px;width:22px;height:22px;border:2px solid #b0f7e2}
.cube:after{content:'• •';position:absolute;top:0;left:2px;letter-spacing:2px}
.spike{position:absolute;bottom:0;left:174px;width:0;height:0;border-left:13px solid transparent;border-right:13px solid transparent;border-bottom:25px solid #b0f7e2}
button{display:block;width:100%;border:0;cursor:pointer;-webkit-tap-highlight-color:transparent;touch-action:manipulation;font-weight:800}
#jump{height:min(31vh,240px);min-height:140px;border-radius:22px;background:#3bddaa;color:#08231a;box-shadow:inset 0 -9px #229b73;font-size:32px;touch-action:none}
#jump:active,#jump.pressed{transform:translateY(3px);box-shadow:inset 0 -4px #229b73;background:#75efc4}
#start{border-radius:12px;min-height:54px;background:#324d71;color:#fff;margin-top:12px;font-size:16px}
.foot{font-size:12px;color:#91a5c2;margin-top:15px}
#connection{color:#79e7c0}
</style>
</head>
<body><main>
<h1>GEOMETRY DASH MINI</h1>
<p>ESP8266 + 1.3 inç OLED | Telefon kontrolcüsü</p>
<div class="box">
<div id="connection">● ESP8266'ya bağlanıldı</div>
<div class="stats">
<div class="stat">SKOR<b id="score">0</b></div>
<div class="stat">REKOR<b id="high">0</b></div>
<div class="stat">SEVİYE<b id="level">1</b></div>
</div>
<div class="display">OYUN OLED EKRANDA GÖRÜNÜR<div class="art"><span class="cube"></span><span class="spike"></span></div></div>
<button id="jump">▲ ZIPLA!</button>
<button id="start">OYUNU BAŞLAT / YENİDEN BAŞLAT</button>
<p id="state">Zıplamak için büyük düğmeye dokun.</p>
</div>
<p class="foot">İnternet gerekmez. Bağlantı sorununda mobil veriyi kapatıp<br>http://192.168.4.1 adresine git.</p>
</main>
<script>
const jump=document.getElementById('jump');
const start=document.getElementById('start');
let waiting=false;
function command(route){fetch(route+'?t='+Date.now(),{cache:'no-store'}).catch(()=>{});}
jump.addEventListener('pointerdown',e=>{
  e.preventDefault(); jump.classList.add('pressed');
  command('/jump'); if(navigator.vibrate) navigator.vibrate(12);
});
for (const type of ['pointerup','pointercancel','pointerleave']) jump.addEventListener(type,()=>jump.classList.remove('pressed'));
jump.addEventListener('contextmenu',e=>e.preventDefault());
start.addEventListener('click',()=>command('/start'));
const states=['Başlat düğmesine bas.','Oyun devam ediyor! Zıpla!','Oyun bitti! Yeniden başlat.'];
async function refresh(){
  if(waiting)return; waiting=true;
  try{
    const resp=await fetch('/status?t='+Date.now(),{cache:'no-store'});
    if(!resp.ok)throw Error('offline');
    const s=await resp.json();
    document.getElementById('score').textContent=s.score;
    document.getElementById('high').textContent=s.high;
    document.getElementById('level').textContent=s.level;
    document.getElementById('state').textContent=states[s.mode]||'';
    document.getElementById('connection').textContent='● ESP8266 bağlantısı aktif';
  }catch(e){document.getElementById('connection').textContent='● ESP8266 bağlantısı kesildi';}
  waiting=false;
}
setInterval(refresh,700);refresh();
</script></body></html>
)HTML";

enum GameMode : uint8_t { MENU=0, PLAYING=1, GAME_OVER=2 };
GameMode gameMode=MENU;

struct Obstacle {
  bool active;
  bool awarded;
  float x;
  int16_t w;
  int16_t h;
  bool block;
};
constexpr uint8_t MAX_OBSTACLES=4;
Obstacle obstacles[MAX_OBSTACLES];

float playerY=FLOOR_Y-PLAYER_SIZE;
float verticalSpeed=0.0f;
bool onGround=true;
float scrollSpeed=1.35f;
float sinceObstacle=0.0f;
float nextObstacleGap=42.0f;
uint16_t distanceOffset=0;
uint32_t score=0;
uint32_t highScore=0;
uint32_t totalFrames=0;
uint8_t level=1;
uint32_t lastPhysics=0;
uint32_t lastDraw=0;
uint32_t storedMarker=0;

void textCentered(const char *s, int16_t y, uint8_t size=1){
  oled.setTextSize(size);
  oled.setCursor((SCREEN_W-(int16_t)strlen(s)*6*size)/2,y);
  oled.print(s);
}

void saveHighScore(){
  if(score<=highScore)return;
  highScore=score;
  EEPROM.put(0, HIGH_SCORE_MAGIC);
  EEPROM.put(4, highScore);
  EEPROM.commit(); // Only on a new record at game over
}

void startGame(){
  gameMode=PLAYING;
  playerY=FLOOR_Y-PLAYER_SIZE;
  verticalSpeed=0.0f;
  onGround=true;
  scrollSpeed=1.35f;
  sinceObstacle=0.0f;
  nextObstacleGap=42.0f;
  distanceOffset=0;
  score=0;
  totalFrames=0;
  level=1;
  for(uint8_t i=0;i<MAX_OBSTACLES;i++) obstacles[i].active=false;
  lastPhysics=millis();
}

void doJump(){
  if(gameMode!=PLAYING){startGame();return;}
  if(onGround){
    verticalSpeed=-5.15f;
    onGround=false;
  }
}

void endGame(){
  if(gameMode!=PLAYING)return;
  gameMode=GAME_OVER;
  saveHighScore();
}

void spawnObstacle(){
  for(uint8_t i=0;i<MAX_OBSTACLES;i++){
    if(obstacles[i].active)continue;
    Obstacle &ob=obstacles[i];
    ob.active=true;
    ob.awarded=false;
    ob.x=SCREEN_W+2;
    ob.block=(score>13 && random(0,4)==0);
    ob.w=ob.block?9:10;
    ob.h=ob.block?13:10;
    break;
  }
}

bool playerHits(const Obstacle &ob){
  const float pLeft=PLAYER_X+2;
  const float pRight=PLAYER_X+PLAYER_SIZE-1;
  const float pTop=playerY+1;
  const float pBottom=playerY+PLAYER_SIZE-1;
  const float oLeft=ob.x+2;
  const float oRight=ob.x+ob.w-2;
  const float oTop=FLOOR_Y-ob.h+(ob.block?0:4); // Forgiving spike hitbox
  return pRight>oLeft && pLeft<oRight && pBottom>oTop && pTop<FLOOR_Y;
}

void updatePhysics(){
  if(gameMode!=PLAYING)return;
  totalFrames++;
  // Gravity; one simulation tick equals 20 milliseconds
  verticalSpeed+=0.47f;
  playerY+=verticalSpeed;
  if(playerY>=FLOOR_Y-PLAYER_SIZE){
    playerY=FLOOR_Y-PLAYER_SIZE;
    verticalSpeed=0;
    onGround=true;
  }
  if(totalFrames%5==0)score++; // Survival points every 100ms
  uint32_t stage = score / 20U;
  if(stage>9U)stage=9U;
  level=1+(uint8_t)stage;
  scrollSpeed=1.35f+min(1.65f,score*0.013f);
  distanceOffset+=(uint16_t)scrollSpeed;
  sinceObstacle+=scrollSpeed;
  if(sinceObstacle>=nextObstacleGap){
    spawnObstacle();
    sinceObstacle=0;
    nextObstacleGap=(float)random(86,117);
  }
  for(uint8_t i=0;i<MAX_OBSTACLES;i++){
    Obstacle &ob=obstacles[i];
    if(!ob.active)continue;
    ob.x-=scrollSpeed;
    if(playerHits(ob)){endGame();return;}
    if(!ob.awarded && ob.x+ob.w<PLAYER_X){ob.awarded=true;score+=5;}
    if(ob.x+ob.w<0)ob.active=false;
  }
}

void drawPlayer(){
  int16_t py=(int16_t)(playerY+0.5f);
  // A little diamond animation during a high jump
  if(!onGround && verticalSpeed>-2.6f && verticalSpeed<2.6f){
    oled.drawLine(PLAYER_X+4,py,PLAYER_X+8,py+4,SH110X_WHITE);
    oled.drawLine(PLAYER_X+8,py+4,PLAYER_X+4,py+8,SH110X_WHITE);
    oled.drawLine(PLAYER_X+4,py+8,PLAYER_X,py+4,SH110X_WHITE);
    oled.drawLine(PLAYER_X,py+4,PLAYER_X+4,py,SH110X_WHITE);
    oled.drawPixel(PLAYER_X+4,py+3,SH110X_WHITE);
  } else {
    oled.drawRect(PLAYER_X,py,PLAYER_SIZE,PLAYER_SIZE,SH110X_WHITE);
    oled.drawPixel(PLAYER_X+2,py+3,SH110X_WHITE);
    oled.drawPixel(PLAYER_X+6,py+3,SH110X_WHITE);
    oled.drawLine(PLAYER_X+3,py+6,PLAYER_X+5,py+6,SH110X_WHITE);
  }
}

void drawGame(){
  oled.setTextSize(1);
  oled.setCursor(1,1);oled.print("S:");oled.print(score);
  oled.setCursor(82,1);oled.print("LV:");oled.print(level);
  oled.drawLine(0,11,127,11,SH110X_WHITE);
  // Stylized background stars
  for(int i=0;i<5;i++){
    int16_t sx=(i*33+21-(totalFrames/5)%33+SCREEN_W)%SCREEN_W;
    int16_t sy=17+(i*11)%19;
    oled.drawPixel(sx,sy,SH110X_WHITE);
  }
  oled.drawLine(0,FLOOR_Y,127,FLOOR_Y,SH110X_WHITE);
  for(int i=0;i<SCREEN_W;i+=18){
    int16_t startX=(i-(totalFrames*2)%18+18)%SCREEN_W;
    oled.drawFastHLine(startX,60,8,SH110X_WHITE);
  }
  drawPlayer();
  for(uint8_t i=0;i<MAX_OBSTACLES;i++){
    const Obstacle &ob=obstacles[i];
    if(!ob.active)continue;
    int16_t x=(int16_t)(ob.x+0.5f);
    int16_t y=FLOOR_Y-ob.h;
    if(ob.block){
      oled.drawRect(x,y,ob.w,ob.h,SH110X_WHITE);
      oled.drawLine(x+2,y+3,x+ob.w-3,y+3,SH110X_WHITE);
    }else{
      oled.drawTriangle(x,FLOOR_Y-1,x+ob.w/2,y,x+ob.w,FLOOR_Y-1,SH110X_WHITE);
    }
  }
}

void render(){
  oled.clearDisplay();
  oled.setTextColor(SH110X_WHITE);
  if(gameMode==MENU){
    textCentered("GEOMETRY",4,2);
    textCentered("DASH MINI",25,1);
    oled.drawRect(17,38,10,10,SH110X_WHITE);
    oled.drawTriangle(72,49,78,37,84,49,SH110X_WHITE);
    oled.drawLine(5,50,123,50,SH110X_WHITE);
    textCentered("WiFi: 192.168.4.1",55,1);
  }else if(gameMode==GAME_OVER){
    textCentered("GAME OVER",3,2);
    oled.setTextSize(1);
    oled.setCursor(12,28);oled.print("SKOR: ");oled.print(score);
    oled.setCursor(12,39);oled.print("REKOR:");oled.print(highScore);
    textCentered("TELEFONDAN BASLAT",55);
  }else{
    drawGame();
  }
  oled.display();
}

void sendStatus(){
  String json;
  json.reserve(82);
  json="{\"mode\":";
  json+=String((unsigned int)gameMode);
  json+=",\"score\":";json+=score;
  json+=",\"high\":";json+=highScore;
  json+=",\"level\":";json+=String((unsigned int)level);
  json+="}";
  server.sendHeader("Cache-Control","no-store");
  server.send(200,"application/json",json);
}

void setup(){
  Serial.begin(115200);
  delay(200);
  Wire.begin(OLED_SDA,OLED_SCL);
  Wire.setClock(400000); // Lower to 100000 if OLED wiring is unstable
  if(!oled.begin(OLED_ADDRESS,true)){
    Serial.println("OLED baslatilamadi! Kablo/I2C adresini kontrol et.");
    while(true){delay(500);}
  }
  oled.setTextColor(SH110X_WHITE);
  EEPROM.begin(32);
  EEPROM.get(0,storedMarker);
  if(storedMarker==HIGH_SCORE_MAGIC){
    EEPROM.get(4,highScore);
    if(highScore>1000000UL)highScore=0;
  }
  randomSeed(micros());
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192,168,4,1),IPAddress(192,168,4,1),IPAddress(255,255,255,0));
  bool started=WiFi.softAP(AP_SSID,AP_PASSWORD);
  Serial.println(started?"ESP8266 Wi-Fi hazir.":"ESP8266 Wi-Fi baslatilamadi!");
  Serial.println(WiFi.softAPIP());
  server.on("/",HTTP_GET,[](){server.send_P(200,PSTR("text/html; charset=utf-8"),PAGE_HTML);});
  server.on("/jump",HTTP_GET,[](){doJump();server.send(200,"text/plain","OK");});
  server.on("/start",HTTP_GET,[](){startGame();server.send(200,"text/plain","OK");});
  server.on("/status",HTTP_GET,sendStatus);
  server.onNotFound([](){server.send(404,"text/plain","Sayfa bulunamadi. http://192.168.4.1");});
  server.begin();
  lastPhysics=millis();
  lastDraw=millis();
  render();
}

void loop(){
  server.handleClient();
  const uint32_t now=millis();
  // Fixed physics updates, capped if web/network stalls momentarily.
  uint8_t steps=0;
  while((uint32_t)(now-lastPhysics)>=FRAME_MS && steps<5){
    updatePhysics();
    lastPhysics+=FRAME_MS;
    steps++;
  }
  if(steps==5)lastPhysics=now;
  if((uint32_t)(now-lastDraw)>=DRAW_MS){
    lastDraw=now;
    render();
  }
  delay(1); // Yield to Wi-Fi / watchdog
}
