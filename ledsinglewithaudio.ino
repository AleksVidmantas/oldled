
//
//#include <ArduinoOTA.h>

#include <FastLED.h>
#include <ESP8266WiFi.h>      // Include the Wi-Fi library

#include <ESP8266HTTPClient.h>
#include <WebSocketsServer.h>
//Declare Spectrum Shield pin connections
#define STROBE D0
#define RESET D1
#define DC_One A0

//Define spectrum variables
int freq_amp;
int Frequencies_One[7];

int i;

CRGB leds[300];
int multi_meteor();
int soundBlend();
int dot_travel_gradual();
int fire_Sound();
const char* ssid     = "Acacia Mesh";         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "Pythagoras";     // The password of the Wi-Fi network
WiFiServer server(80);
//int (*pattern)() = multi_meteor;
int fivePeaks();
//int (*pattern)() = soundBlend;
//int (*pattern)()=fivePeaks;

int (*pattern)()=dot_travel_gradual;


//parameter variables
bool flop = false;
int flex_ind = 0;
int flex_gen1 = 0;
int flex_gen2 = 0;
int flex_gen3 = 0;
int flex_gen4 = 0;
int ind = 0;
int hue = 0;
int hsv_value = 220;
int leads[] = {0, 100, 200, 200}; //used in mantis() aka multi meteor
WebSocketsServer webSocket(81);    // create a websocket server on port 81
struct FocalNode {

  int brightness;
  bool climbPhase;
  bool canBeReset;
  int index;

};
FocalNode nodes[20];

int mask1[300] = {0};
int mask1Offset = 0;
int fivePeaksAr[300] = {0};

void startWebSocket();
void setup() {
  for(int i = 0; i<25; i++){
    fivePeaksAr[50 +i] = 255-i*4;
    fivePeaksAr[50-i] =random(190,250)-i*8;
    fivePeaksAr[100+i] =random(190,250)-i*8;
    fivePeaksAr[100-i] =random(190,250)-i*8;
    fivePeaksAr[150+i] =random(190,250)-i*8;
    fivePeaksAr[150-i] =random(190,250)-i*8;
    fivePeaksAr[200+i] =random(190,250)-i*8;
    fivePeaksAr[200-i] =random(190,250)-i*8;
    fivePeaksAr[250+i] =random(190,250)-i*8;
    fivePeaksAr[250-i] =random(190,250)-i*8;
  }
  
  for(int i = 0; i < 149; i ++){
    mask1[i] = random(160,211)-2*i;
    mask1[300-i] = random(160,21)-2*i;
  }
  
    //Set spectrum Shield pin configurations
  pinMode(STROBE, OUTPUT);
  pinMode(RESET, OUTPUT);
  pinMode(DC_One, INPUT);
//  pinMode(DC_Two, INPUT)
  //Initialize Spectrum Analyzers
  digitalWrite(STROBE, LOW);
  digitalWrite(RESET, LOW);
  delay(5);

  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');

  WiFi.begin(ssid, password);             // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i<=20) { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i); Serial.print(' ');

  }

  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer
  FastLED.addLeds<WS2812B, 12, GRB>(leds, 300);
  startWebSocket();            // Start a WebSocket server

  server.begin();
  for (int i = 0; i < 20; i++) { //initalize nodes with starting nodes
    nodes[i].brightness = 0;
    nodes[i].climbPhase = true;
    nodes[i].canBeReset = true;
    nodes[i].index = 0;
  }
}


void Graph_Frequencies();
void Read_Frequencies();

int timeBuffer = 3;

bool skipLoop = false;

void loop() {

  webSocket.loop();
   Read_Frequencies();
  Graph_Frequencies();

  if (timeBuffer < 5) { //safety delay to prevent double calls
    delay(1);
    timeBuffer++;
  } else {
    timeBuffer = 0;
    WiFiClient client = server.available();   // Listen for incoming clients

    if (client) {                             // If a new client connects,
      Serial.println("\n[Client connected]");
      String total_lines = "";
      while (client.connected())
      {
        // read line by line what the client (web browser) is requesting
        if (client.available())
        {
          String line = client.readStringUntil('\r');
          total_lines += line;

          // Serial.print(line);
          // wait for end of client's request, that is marked with an empty line
          if (line.length() == 1 && line[0] == '\n')
          {
            /*    Unnecessary for now since we only read requests
                  client.println("HTTP/1.1 200 OK");
                  client.println("Content-type:text/html");
                  client.println("Connection: close");
                  client.println();

                  client.println("<!DOCTYPE html><html>");
                  client.println("<body><h1>ESP8266 Web Server</h1>");
                  client.println("</body></html>");
                  client.println();
                  Serial.print("Val is: ");
                  Serial.println(clientHasConnected);
            */
            break;
          }
        }
        timeBuffer = 0;
      }
      client.flush();
      client.stop();

      Serial.println("[Client Disconnected]");
      Serial.println("Resulting lines...");
      Serial.println(total_lines);
      if (total_lines.indexOf("/set") >= 0) {
        set_val_handler(total_lines);

        //          pattern = purple_sparse_flicker;
        //        }else if(total_lines.indexOf("GET /set/pattern/1") >= 0 && pattern!=perlin1){
        //          pattern = perlin1;
        //        }else if(total_lines.indexOf("GET /set/pattern/2") >= 0 && pattern!=multi_meteor){
        //          pattern = multi_meteor;
        //        }
      } else if (total_lines.indexOf("GET /set/off/fade") >= 0 ) {
        pattern = fade_off;
      }
    }
  }
  //  server.handleClient();
  if (skipLoop) {
    skipLoop = false;
  } else {
    pattern();
    FastLED.show();
    delay(1);
  }


  //Serial.println("Loop");
}


/*************Pull frquencies from Spectrum Shield****************/
void Read_Frequencies() {
  digitalWrite(RESET, HIGH);
  delayMicroseconds(200);
  digitalWrite(RESET, LOW);
  delayMicroseconds(200);

  //Read frequencies for each band
  for (freq_amp = 0; freq_amp < 7; freq_amp++)
  {
    digitalWrite(STROBE, HIGH);
    delayMicroseconds(50);
    digitalWrite(STROBE, LOW);
    delayMicroseconds(50);

    Frequencies_One[freq_amp] = analogRead(DC_One);
//    Frequencies_Two[freq_amp] = analogRead(DC_Two);
  }
}
/*****************Print Out Band Values for Serial Plotter*****************/
void Graph_Frequencies() {
  for (i = 0; i < 7; i++)
  {
//    Serial.print(Frequencies_One[i]);
//    Serial.print(" ");
//    Serial.print(Frequencies_Two[i]);
//    Serial.print(" ");
    Serial.print( Frequencies_One[i] );
    Serial.print(" ");
  }
  Serial.println();
}
void resetVals(){
  //parameter variables
  flop = false;
  flex_ind = 0;
  flex_gen1 = 0;
  flex_gen2 = 0;
  flex_gen3 = 0;
  flex_gen4 = 0;
  ind = 0;
  hsv_value = 255;
  hue = 0;
}
//ex url = brght=200
void set_val_handler(String url) { //can genericize all of this
  int initialIndex = 0; //url.indexOf("?");
  //Serial.print("Query String is "); get rid of println for performance

  int endQuery = url.indexOf(' ', initialIndex);
  String query = url.substring(initialIndex, endQuery);
  //Serial.println(query);

  int patternFlag = query.indexOf("ptrn");
  int brightnessFlag = query.indexOf("brght");
  int hueFlag = query.indexOf("hue");
  int satFlag = query.indexOf("sat");
  int gen1Flag = query.indexOf("gen1");
  int gen2Flag = query.indexOf("gen2");
  int gen3Flag = query.indexOf("gen3");
  int gen4Flag = query.indexOf("gen4");
  if (patternFlag >= 0) {
    FastLED.clear();
    FastLED.show();
    int valBegin = query.indexOf('=', patternFlag);
    int valEnd;
    (query.indexOf('?') >= 0) ? valEnd = query.indexOf('?') : valEnd = query.length();
    String patternVal = query.substring(valBegin + 1, valEnd);
    Serial.println(patternVal);
    int ptrnInt = patternVal.toInt();
    resetVals();
    if (ptrnInt == 0) {
      pattern = purple_sparse_flicker;
    } else if (ptrnInt == 1) {
      pattern = perlin1;
    } else if (ptrnInt == 2) {
      pattern = multi_meteor;
    } else if (ptrnInt == 3) {
      pattern = paparazzi;
    } else if (ptrnInt == 4) {
      pattern = bars;
    } else if (ptrnInt == 5) {
      pattern = shum6;
    } else if (ptrnInt == 6) {
      pattern = sparkle_gradual;
    } else if (ptrnInt == 7) {
      pattern = bars;
    } else if (ptrnInt == 8) {
      pattern = soundBlend;
    } else if (ptrnInt == 9) {
      pattern = dot_travel_gradual;
    } else if (ptrnInt == 10) {
      pattern = sparkle_constant;
    } else if (ptrnInt == 11) {
//      pattern = sin_vibe;
    } else if (ptrnInt == 12){
      pattern = same_meteor;
    } else if (ptrnInt == 13){
      pattern = dot_travel_gradual;
    } else if(ptrnInt ==14){
//      pattern = sin_vibe_inc;
    }
  }

  //could modularize this
  if (brightnessFlag >= 0) {
    int valBegin = query.indexOf('=', brightnessFlag);
    int valEnd;
    (query.indexOf('?') >= 0) ? valEnd = query.indexOf('?') : valEnd = query.length();
    String brightnessVal = query.substring(valBegin + 1, valEnd);
    FastLED.setBrightness(brightnessVal.toInt());
  }
  if (hueFlag >= 0) {
    int valBegin = query.indexOf('=', hueFlag);
    int valEnd;
    (query.indexOf('?') >= 0) ? valEnd = query.indexOf('?') : valEnd = query.length();
    String hueVal = query.substring(valBegin + 1, valEnd);
    hue = hueVal.toInt();
  }
  if (satFlag >= 0) {
    int valBegin = query.indexOf('=', satFlag);
    int valEnd;
    (query.indexOf('?') >= 0) ? valEnd = query.indexOf('?') : valEnd = query.length();
    String satVal = query.substring(valBegin + 1, valEnd);
    hsv_value = satVal.toInt();
  }
  if (gen1Flag >= 0) {
    int valBegin = query.indexOf('=', gen1Flag);
    int valEnd;
    (query.indexOf('?') >= 0) ? valEnd = query.indexOf('?') : valEnd = query.length();
    String gen1Val = query.substring(valBegin + 1, valEnd);
    flex_gen1 = gen1Val.toInt();
  }
  if (gen2Flag >= 0) {
    int valBegin = query.indexOf('=', gen2Flag);
    int valEnd;
    (query.indexOf('?') >= 0) ? valEnd = query.indexOf('?') : valEnd = query.length();
    String gen2Val = query.substring(valBegin + 1, valEnd);
    flex_gen2 = gen2Val.toInt();
  }
  if (gen3Flag >= 0) {
    int valBegin = query.indexOf('=', gen3Flag);
    int valEnd;
    (query.indexOf('?') >= 0) ? valEnd = query.indexOf('?') : valEnd = query.length();
    String gen3Val = query.substring(valBegin + 1, valEnd);
    flex_gen3 = gen3Val.toInt();
  }
  if (gen4Flag >= 0) {
    int valBegin = query.indexOf('=', gen4Flag);
    int valEnd;
    (query.indexOf('?') >= 0) ? valEnd = query.indexOf('?') : valEnd = query.length();
    String gen4Val = query.substring(valBegin + 1, valEnd);
    flex_gen4 = gen4Val.toInt();
  }

}

int fire_Sound(){

  for(int i = 0; i < 300; i++){
    leds[i] = CHSV(Frequencies_One[2]/3,255,Frequencies_One[2]/3);
  }
  FastLED.setBrightness(Frequencies_One[2]/3);
  
  return 0;
}

int fivePeaks(){
  for(int i = 0; i < 300; i++){
    leds[min(300,max((i+sin8(flex_ind)/6),0))] = CHSV(map(inoise8(i * 5000, flex_gen1), 0, 255, 160, 220)+hue, 255, fivePeaksAr[i+mask1Offset%10]);
//  leds[i] = CHSV(155, 255, fivePeaksAr[i+mask1Offset%10]);
  }
    FastLED.setBrightness((double)Frequencies_One[0]/3);
//  mask1Offset++;
   flex_ind++;
   flex_gen1+=2000;
  return 0;
}
//flex_ind = 0;
int soundBlend(){
  for(int i = 0; i < 150; i++){
    leds[i] = CHSV(Frequencies_One[2]/11+170+i+flex_ind, 255,min((double)225,(double)mask1[i+mask1Offset%10])) ;
    leds[300-i] = CHSV(Frequencies_One[2]/11+170+i+flex_ind, 255,min((double)225,(double)mask1[i-mask1Offset%10])) ;

  }
  if(flex_ind > 90){
    flop = true;
    
  }else if(flex_ind < 1){
    flop = false;
  }
  if(flop){
    flex_ind--;  
  }else{
    flex_ind++;
  }
  
    
  mask1Offset++;
  FastLED.setBrightness((double)Frequencies_One[0]/3);
  return 0;
}
int fade_off() {
  for (int i = 0; i < 300; i++) {
    leds[i].fadeToBlackBy(12);
  }
  return 0;
}
int purple_sparse_flicker() {
  for (int i = 0; i < 300; i += 10) {
    if (i < 300) {
      leds[i] = CHSV(random(192, 224), random(200, 255), random(150, 255));
    }

  }
  return 0;
}

int perlin1() {
  for (int i = 0; i < 300; i++) {
    leds[i] = CHSV(inoise16(i * 5000, flex_ind) / 255, 255, 255);

  }
  hue++;
  flex_ind += 2000;
  return 0;
}



int multi_meteor() { //also called multimeteor
  for (int i = 0; i < 3; i++) {
    if (leads[i] > 299) {
      leads[i] = 0;
    }
    
    leads[i]++;
    leds[leads[i]] = CHSV(hue, hsv_value, max(Frequencies_One[0]/3,50));
  }
  for (int i = 0; i < 300; i++) {
    //leds[i+200] = CRGB(255, 255, 255);
    //leds[i] = CRGB(0, strength * 255, 0);
    leds[i].fadeToBlackBy(4);
  }
  hue++;
  

  return 0;
}
//
//int same_meteor() { //also called multimeteor
//  for (int i = 0; i < 3; i++) {
//    if (leads[i] > 299) {
//      leads[i] = 0;
//    }
//    leads[i]++;
//    leds[leads[i]] = CHSV(hue, hsv_value, random(150, 255));
//  }
//  for (int i = 0; i < 300; i++) {
//    //leds[i+200] = CRGB(255, 255, 255);
//    //leds[i] = CRGB(0, strength * 255, 0);
//    leds[i].fadeToBlackBy(4);
//  }
// 
//  return 0;
//}
int usedHue = 170;
bool reversed = false;
int dot_travel_gradual() {
    leds[i].fadeToBlackBy(155);
    if(reversed){
      usedHue++;
    }else{
      usedHue--;
    }

    if(usedHue >= 244){
      reversed = !reversed;
    }
    if(usedHue < 75){
      reversed = !reversed;
    }

  for (int i = 0; i < 10; i++) {
    if ((flex_ind + 30 * i) - 1 == 299) {
//      leds[299] = CHSV(0, 0, 0);
    }
    if (flex_ind + 30 * i >= 300) {
      leds[(flex_ind + 30 * i) - 300] = CHSV(usedHue, hsv_value,  max(Frequencies_One[0]/3,50));
//      leds[(flex_ind + 30 * i) - 300 - 1] = CHSV(0, 0, 0);
      //      leds[299] = CHSV(0,0,0);

    } else {
      leds[flex_ind + 30 * i] = CHSV(usedHue, hsv_value,  max(Frequencies_One[0]/3,50));
      leds[flex_ind+1 + 30 * i] = CHSV(usedHue, hsv_value, max(Frequencies_One[0]/3,50));
        leds[flex_ind-1 + 30 * i] = CHSV(usedHue, hsv_value, max(Frequencies_One[0]/3,50));

//      leds[flex_ind + 30 * i - 1] = CHSV(0, 0, 0);
    }

  }

  if (flex_ind > 299) {
    int (*pattern)() = purple_sparse_flicker;
//    leds[299] = CHSV(0, 0, 0);
    flex_ind = 0;
  }
  flex_gen1++;
    blur1d(leds, 300, 20);


  //  leds[flex_ind] = CHSV(hue, 140, BRIGHTNESS);
  //  leds[flex_ind -1] = CHSV(0,0,0);
  flex_ind++;
  return 0;
}

int shum6() {
  static byte t;
#define LEN_BARS 50
  for (short i = 0; i < 300; i += LEN_BARS) {
    short center_idx = i + (LEN_BARS / 2);
    leds[center_idx + t] = CHSV(hue, hsv_value,  max(Frequencies_One[0]/3,50));
    leds[center_idx - t] = CHSV(hue, hsv_value,  max(Frequencies_One[0]/3,50));
    //    Serial.println(center_idx);
    //    Serial.println(t);
  }
  for (int i = 0; i < 300; i++) {
    leds[i].fadeToBlackBy(20);
  }
  if (++t >= LEN_BARS / 2)
    t = 0;
  hue++;
  return 0;
}
int constant_meteor() { //also called multimeteor
  for (int i = 0; i < 3; i++) {
    if (leads[i] > 299) {
      leads[i] = 0;
    }
    leads[i]++;
    leds[leads[i]] = CHSV(hue, hsv_value,  min((Frequencies_One[0]-100)/1.99+(Frequencies_One[1]-100)/1.99,(double)255));
  }
  for (int i = 0; i < 300; i++) {
    //leds[i+200] = CRGB(255, 255, 255);
    //leds[i] = CRGB(0, strength * 255, 0);
    leds[i].fadeToBlackBy(4);
  }
  //hue++;
  return 0;
}

int same_meteor() { //also called multimeteor
  for (int i = 0; i < 3; i++) {
    if (leads[i] > 299) {
      leads[i] = 0;
    }
    leads[i]++;
    leds[leads[i]] = CHSV(hue, hsv_value,  min((Frequencies_One[0]-100)/1.99+(Frequencies_One[1]-100)/1.99,(double)255));
    
  }
  for (int i = 0; i < 300; i++) {
    //leds[i+200] = CRGB(255, 255, 255);
    //leds[i] = CRGB(0, strength * 255, 0);
    leds[i].fadeToBlackBy(4);
  }

 
  return 0;
}

int shum5() {
  static byte t;
#define LEN_BARS 50
  for (short i = 0; i < 300; i += LEN_BARS) {
    short center_idx = i + (LEN_BARS / 2);
    leds[center_idx + t] = CHSV(hue, hsv_value, max(Frequencies_One[0]/3,50));
    leds[center_idx - t] = CHSV(hue, hsv_value, max(Frequencies_One[0]/3,50));
    //    Serial.println(center_idx);
    //    Serial.println(t);
  }
  for (int i = 0; i < 300; i++) {
    leds[i].fadeToBlackBy(20);
  }
  if (++t >= LEN_BARS / 2)
    t = 0;
  hue++;
  return 0;
}


int shum1() {
  static byte t;
#define LEN_BARS 50
  for (short i = 0; i < 300; i += LEN_BARS) {
    short center_idx = i + (LEN_BARS / 2);
    leds[center_idx + t] = CHSV(hue, hsv_value, 200);
    leds[center_idx - t] = CHSV(hue, hsv_value, 200);
    //    Serial.println(center_idx);
    //    Serial.println(t);
  }
  for (int i = 0; i < 300; i++) {
    leds[i].fadeToBlackBy(20);
  }
  if (++t >= LEN_BARS / 2)
    t = 0;
  return 0;
}

int sparkle_gradual() {

  //  leds[random(0,200)] = CHSV(140, 200, random(150,255));
  //  leds[random(0,300)] = CHSV(140, 200, random(150,255));
  //  leds[random(0,300)] = CHSV(random(170,200), random(200,255), random(150,255));
  static byte t;
  for (int i = 0; i < 3; i++) {
    leds[random(0, 300)] = CHSV(t, hsv_value,  max(Frequencies_One[0]/3,50));
  }
  for (int i = 0; i < 300; i++) {
    //leds[i+200] = CRGB(255, 255, 255);
    //leds[i] = CRGB(0, strength * 255, 0);
    leds[i].fadeToBlackBy(10);
  }
  t++;
  blur1d(leds, 300, 20);
//    FastLED.setBrightness(min((Frequencies_One[0]-100)/1.99+(Frequencies_One[1]-100)/1.99,(double)255));

  return 0;
}

int sin_vibe() {
  for (int i = 0; i < 300; i++) {
    leds[i] = CHSV(hue, hsv_value,min(1*sin8((i/max((float)(max((float)flex_gen2,1.0f)/85.0f),0.000001f)+((255-flex_gen2)*flex_gen1))),255));
    //Serial.println(sin8((i+flex_gen1)/flex_gen2));
  }
}
int sin_vibe_inc() {
  
  for (int i = 0; i < 300; i++) {
    leds[i] = CHSV(hue, hsv_value,min(1*sin8((i/max((float)(max((float)flex_gen2,1.0f)/85.0f),0.000001f)+((255-flex_gen2)*flex_gen1))),255));
    //Serial.println(sin8((i+flex_gen1)/flex_gen2));
  }
  flex_gen1++;
}

int sparkle_constant() {
  //  leds[random(0,200)] = CHSV(140, 200, random(150,255));
  //  leds[random(0,300)] = CHSV(140, 200, random(150,255));
  //  leds[random(0,300)] = CHSV(random(170,200), random(200,255), random(150,255));
  static byte t;
  for (int i = 0; i < 3; i++) {
    leds[random(0, 300)] = CHSV(hue, hsv_value, random(150, 255));
  }
  for (int i = 0; i < 300; i++) {
    //leds[i+200] = CRGB(255, 255, 255);
    //leds[i] = CRGB(0, strength * 255, 0);
    leds[i].fadeToBlackBy(10);
  }
  blur1d(leds, 300, 20);
  return 0;
}

int paparazzi() { //2 leds that fltmpBassash white then off rapidly like camera flash
  int tmp1 = random(0, 300);
  int tmp2 = random(0, 300);
  //  int tmp3 = random(0, 300);
  //  int tmp4 = random(0, 300);



  leds[tmp1] = CRGB(255, 255, 255);
  leds[tmp2] = CRGB(255, 255, 255);
  FastLED.show();
  delay(1);
  leds[tmp1] = CRGB(0, 0, 0);
  leds[tmp2] = CRGB(0, 0, 0);
  return 0;
}

int bars() {
  //  hue = random(0,255);
  byte hue = random(96,224);
//  byte val = min(255, (int)random(0, 50) + hsv_value - 25);

  ind = random(0, 270);
  //  Serial.println(BRIGHTNESS);
  //  Serial.println(" ");

  for (int i = 0; i < 30; i++) {
    leds[ind + i] = CHSV(hue, 254,  max(Frequencies_One[0]/3,50));
  }

  //  leds[0] = CRGB(0, 255, 255);
  for (int i = 0; i < 300; i++) {
    //leds[i+200] = CRGB(255, 255, 255);
    //leds[i] = CRGB(0, strength * 255, 0);
    leds[i].fadeToBlackBy( 64 );
  }
  return 0;
}


void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");
}

// Minimal class to replace std::vector
template<typename Data>
class Vector {
    size_t d_size; // Stores no. of actually stored objects
    size_t d_capacity; // Stores allocated capacity
    Data *d_data; // Stores data
  public:
    Vector() : d_size(0), d_capacity(0), d_data(0) {}; // Default constructor
    Vector(Vector const &other) : d_size(other.d_size), d_capacity(other.d_capacity), d_data(0) {
      d_data = (Data *)malloc(d_capacity * sizeof(Data));
      memcpy(d_data, other.d_data, d_size * sizeof(Data));
    }; // Copy constuctor
    ~Vector() {
      free(d_data);
    }; // Destructor
    Vector &operator=(Vector const &other) {
      free(d_data);
      d_size = other.d_size;
      d_capacity = other.d_capacity;
      d_data = (Data *)malloc(d_capacity * sizeof(Data));
      memcpy(d_data, other.d_data, d_size * sizeof(Data));
      return *this;
    }; // Needed for memory management
    void push_back(Data const &x) {
      if (d_capacity == d_size) resize();
      d_data[d_size++] = x;
    }; // Adds new value. If needed, allocates more space
    size_t size() const {
      return d_size;
    }; // Size getter
    Data const &operator[](size_t idx) const {
      return d_data[idx];
    }; // Const getter
    Data &operator[](size_t idx) {
      return d_data[idx];
    }; // Changeable getter
  private:
    void resize() {
      d_capacity = d_capacity ? d_capacity * 2 : 1;
      Data *newdata = (Data *)malloc(d_capacity * sizeof(Data));
      memcpy(newdata, d_data, d_size * sizeof(Data));
      free(d_data);
      d_data = newdata;
    };// Allocates double the old space
};

int randomSectionFadeInOut() {

  for (int i = 0; i < 20; i++) {
    if (random(0, 50) == 4) {
      if (nodes[i].canBeReset == true) { //Everything in this if statement applies to an led that has been extinguished
        nodes[i].climbPhase = true;
        nodes[i].brightness = 0;
        nodes[i].canBeReset = false;

        //block to determine an index for a properly spaced led
        Vector<int> validIndexes;
        for (int j = 0; j < 300; j++) {
          if (j < 6) {

          }
          bool indexValid = true;
          for (int k = 0; k < 21; k++) { //5 for 5 indexes, 6 for adding number to valid index
            if (k < 20) {
              //these if statements check if the index is invalid.
              if (nodes[k].index > j) {
                if (!((nodes[k].index - j) > 20)) {
                  indexValid = false;
                }
              } else {
                if (!((j - nodes[k].index) > 20)) {
                  indexValid = false;
                }
              }
            } else {
              validIndexes.push_back(j);
            }

          }
        }

        nodes[i].index = validIndexes[(random(0, validIndexes.size() - 1))]; //assign led a valid spaced index

        for (int j = 0; j < 2; j++) {

        }


      }
    }


    if (nodes[i].canBeReset == false) { //only act on active leds
      if (nodes[i].brightness > 255) {
        nodes[i].climbPhase = false;
      }

      if (nodes[i].climbPhase) {
        nodes[i].brightness += 8;
      } else {
        nodes[i].brightness -= 3;
        if (nodes[i].brightness < 1) {
          nodes[i].canBeReset = true;
        }
      }
      if (nodes[i].brightness < 0) {
        nodes[i].brightness = 0;
      }
      leds[nodes[i].index] = CHSV(hue, hsv_value, min(nodes[i].brightness, 255));
    }

    blur1d(leds, 300, 150);
    //    FastLED.show();
  }
  hue++;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  skipLoop = true;
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);


      }
      break;
    case WStype_TEXT:                     // if new text data is received
      //      Serial.printf("[%u] get Text: %s\n", num, payload);
      String pl = (char*)payload;
      set_val_handler(pl);

      break;
  }
}
