#include <Adafruit_Protomatter.h>
#include <Adafruit_GFX.h>

// === MATRIX PORTAL M4 PIN CONFIGURATION ===
uint8_t rgbPins[]  = {7, 8, 9, 10, 11, 12};
uint8_t addrPins[] = {17, 18, 19, 20, 21};
uint8_t clockPin   = 14;
uint8_t latchPin   = 15;
uint8_t oePin      = 16;

// Initialize matrix: 64x32, 4-bit color depth (4096 colors)
Adafruit_Protomatter matrix(
  64, 4, 1, rgbPins, 4, addrPins, clockPin, latchPin, oePin, false
);

// === WAVEFORM PARAMETERS (ADAPTED FROM ARDUBOY CODE) ===
const int ADC_PIN = A2;           // Using A2 as you originally specified
int sample = 0;
int lastsample = 0;
int newy = 0;
int yoffset = -6;                   // Will adjust for 32-pixel height
int ampVal = 6;                    // Starting amplitude multiplier

// For controlling frame rate
unsigned long lastFrameTime = 0;
const int FRAME_DELAY_MS = 16;     // ~60 FPS (1000/60 ≈ 16.67)

// === RAINBOW MARCH EFFECT VARIABLES ===
unsigned long lastHueShift = 0;
const int HUE_SHIFT_DELAY_MS = 30;
uint8_t hueOffset = 0;
struct Wave {
  int x;
  int y;
};
Wave wave = {0,0};
// Store the entire waveform for rainbow coloring
int waveformY[64];  // Buffer for 64 horizontal positions

// Oversampling configuration
const int OVERSAMPLING_RATE = 20;  // Samples per display column
const int SMOOTHING_WINDOW = 6;    // Optional: moving average window size
int sampleHistory[64];               // For optional moving average

void setup(void) {
  Serial.begin(115200);
  Serial.println("MatrixPortal M4 - Waveform Display (Adapted from Arduboy)");
  
  // Initialize the matrix
  ProtomatterStatus status = matrix.begin();
  Serial.print("Protomatter begin() status: ");
  Serial.println((int)status);
  
  if (status != PROTOMATTER_OK) {
    Serial.println("Matrix initialization failed!");
    while (1);  // Halt if matrix fails
  }
  
  // Set up ADC for MatrixPortal M4
  analogReadResolution(8);  // 8-bit resolution (0-255)
  
  // Initialize sample history for moving average
  for (int i = 0; i < 64; i++) {
    sampleHistory[i] = 0;
  }
  
  Serial.println("Setup complete. Displaying waveform...");
}

void loop(void) {
  // Frame rate control (similar to Arduboy.nextFrame())
  unsigned long currentTime = millis();
  
  lastFrameTime = currentTime;
  
  // Update rainbow hue offset
  if (currentTime - lastHueShift >= HUE_SHIFT_DELAY_MS) {
    lastHueShift = currentTime;
    hueOffset = (hueOffset + 3) % 256;  // Increment hue
  }
  
  // Update waveform display
  displayWave();
}

void displayWave(void) {
  // Clear the entire screen to black every frame
  matrix.fillScreen(0);
  
  // For each column on the 64-pixel wide screen...
  for (wave.x = 0; wave.x < 64; wave.x++) {
    // === OVERSAMPLING: Take multiple samples and average ===
    long sampleSum = 0;
    
    // Take OVERSAMPLING_RATE samples for this column
    for (int i = 0; i < OVERSAMPLING_RATE; i++) {
      sampleSum += analogRead(ADC_PIN);
    }
    
    // Calculate the average
    sample = sampleSum / OVERSAMPLING_RATE;
    
    // === OPTIONAL: Apply moving average for additional smoothing ===
    // You can enable this if you want even smoother waveforms
    
    sampleHistory[wave.x] = sample;
    
    // Calculate moving average over last SMOOTHING_WINDOW samples
    long movingSum = 0;
    int count = 0;
    for (int i = max(0, wave.x - SMOOTHING_WINDOW + 1); i <= wave.x; i++) {
      movingSum += sampleHistory[i];
      count++;
    }
    sample = movingSum / count;
    
    
    // === ADAPTED MAPPING FOR 32-PIXEL HEIGHT ===
    // Map the averaged sample to display coordinates
    // Adjust these mapping parameters based on your signal characteristics
    newy = map(sample, 0, 255, 0, 32);
    
    // Alternative: Use the original formula with oversampled value
    // newy = (((sample * ampVal)/64) - yoffset);
    
    // Constrain to screen bounds (0-31)
    newy = constrain(newy, 0, 31);
    
    // Store in buffer for this column
    wave.y = newy;
    
    // Draw a vertical line at this column, from last sample to current
    if (wave.x > 0) {
      // Calculate rainbow color based on x position and hue offset
      uint8_t hue = ((wave.x * 4) + hueOffset) % 256;
      uint32_t rgbColor = hueToRGB(hue);
      
      // Convert to 16-bit color for Protomatter
      uint16_t lineColor = matrix.color565(
        (rgbColor >> 16) & 0xFF,
        (rgbColor >> 8)  & 0xFF,
        rgbColor         & 0xFF
      );
      
      // Draw vertical line between the two points
      matrix.drawLine(wave.x, lastsample, wave.x, wave.y, lineColor);
    }
    
    // For debugging: Print to Serial (like Arduboy's println)
    if (wave.x == 0) {  // Only print once per frame to avoid Serial spam
      Serial.print("Sample: ");
      Serial.print(sample);
      Serial.print(" | Mapped Y: ");
      Serial.println(newy);
      Serial.print("Oversampling: ");
      Serial.print(OVERSAMPLING_RATE);
      Serial.println("x");
    }
    
    lastsample = newy;
  }
  
  // Update the physical matrix
  matrix.show();
}

// Converts hue (0-255) to RGB color (Adafruit rainbow wheel)
uint32_t hueToRGB(uint8_t hue) {
  hue = 255 - hue;  // Reverse for preferred color order
  
  if (hue < 85) {
    // Red to Green
    return ((uint32_t)(255 - hue * 3) << 16) | ((uint32_t)(hue * 3) << 8);
  } else if (hue < 170) {
    // Green to Blue
    hue -= 85;
    return ((uint32_t)(hue * 3) << 8) | (255 - hue * 3);
  } else {
    // Blue to Red
    hue -= 170;
    return ((uint32_t)(hue * 3) << 16) | (255 - hue * 3);
  }
}

// === CONTROLS FOR ADJUSTING DISPLAY ===
// You can call these from loop() based on button presses or Serial commands

void increaseAmplitude() {
  ampVal = min(ampVal + 1, 10);  // Limit to 10x amplification
  Serial.print("Amplitude: ");
  Serial.println(ampVal);
}

void decreaseAmplitude() {
  ampVal = max(ampVal - 1, 1);  // Don't go below 1
  Serial.print("Amplitude: ");
  Serial.println(ampVal);
}

void adjustOffset(int delta) {
  yoffset += delta;
  Serial.print("Y Offset: ");
  Serial.println(yoffset);
}