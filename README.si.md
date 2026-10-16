# ESP32 Firebase LED රටා පාලකය

ESP32 එකකට සම්බන්ධ කරපු LED 6ක් web පිටුවකින් Firebase Realtime Database හරහා පාලනය කරන project එකක්. Built-in රටාවක් තෝරන්න, ඒකේ වේගය හදන්න, නැත්නම් ඔයාටම රටාවක් step එකින් එක හදලා හැම step එකකටම වෙන වෙනම කාලයක් දෙන්න පුළුවන්. තෝරපු රටාව ESP32 එකේ flash memory එකේ save වෙන නිසා reboot උනත්, WiFi නැතත් ඒක දිගටම දුවනවා.

## විශේෂාංග

- LED 6ක් සහ built-in රටා 6ක් (blink, chase, Knight Rider, alternate, random, off)
- **Custom රටා**: step 300ක් දක්වා. හැම step එකකටම LED mask එකක් සහ ms වලින් කාලයක්
- **Flash එකේ save වෙනවා**: අන්තිම රටාව NVS එකේ save වෙලා boot එකේදී ආපහු load වෙනවා
- **ලැග් නැහැ**: Firebase බලන එක core 0 එකේ වෙනම FreeRTOS task එකක දුවන නිසා, LED animation එක (core 1) කවදාවත් නතර වෙන්නේ නැහැ
- **Web controller එක**: framework එකක් හෝ build step එකක් නැති එකම `index.html` එකක්, step builder එකත් එක්ක
- රටාව වෙනස් උනොත් විතරක් flash එකට ලියන නිසා flash එක ගෙවෙන්නේ අඩුවෙන්

## Repository එකේ ව්‍යුහය

```
.
├── esp32-firmware/
│   └── esp32-firmware.ino     # Arduino sketch (folder එකේ නමයි file එකේ නමයි සමාන වෙන්න ඕන)
├── web-app/
│   └── index.html             # Web controller
└── README.md
```

## Hardware

- ESP32 development board එකක්
- LED 6ක්
- Resistor 6ක් (220 Ω සිට 330 Ω)
- Breadboard සහ jumper wires

### Default wiring

| Mask එකේ ස්ථානය | GPIO |
|---|---|
| 1 | 2 |
| 2 | 4 |
| 3 | 5 |
| 4 | 18 |
| 5 | 19 |
| 6 | 21 |

හැම GPIO එකක්ම resistor එකක් හරහා LED එකේ anode එකට (දිගු කකුල) සම්බන්ධ කරන්න. Cathode එක (කෙටි කකුල) GND එකට සම්බන්ධ කරන්න.

Pins වෙනස් කරන්න sketch එකේ මේ පේළිය වෙනස් කරන්න. පිළිවෙල වැදගත්, මොකද mask එකේ පළවෙනි අකුර array එකේ පළවෙනි pin එකට අදාළයි:

```cpp
const uint8_t PINS[] = {2, 4, 5, 18, 19, 21};
```

ආරක්ෂිතව පාවිච්චි කරන්න පුළුවන් pins: `13, 14, 16, 17, 22, 23, 25, 26, 27, 32, 33`. GPIO 6-11 (flash එකට සම්බන්ධයි), 34-39 (input විතරයි), 0, 12, 15 (boot strapping pins) පාවිච්චි කරන්න එපා.

## Setup

### 1. Firebase

1. [Firebase console](https://console.firebase.google.com) එකේ project එකක් හදන්න.
2. **Build → Realtime Database → Create Database** ගිහින් location එකක් තෝරන්න.
3. Database URL එක copy කරන්න. ඒක `https://your-project-default-rtdb.firebasedatabase.app` වගේ එකක්.
4. **Rules** tab එකේ මේක paste කරලා **Publish** ඔබන්න:

```json
{
  "rules": {
    "led": {
      ".read": true,
      ".write": true,
      "pattern": { ".validate": "newData.isNumber() && newData.val() >= 0 && newData.val() <= 6" },
      "speed":   { ".validate": "newData.isNumber() && newData.val() >= 20 && newData.val() <= 5000" },
      "custom":  { ".validate": "newData.isString() && newData.val().length <= 3900" }
    }
  }
}
```

5. (අවශ්‍ය නම්) ආරම්භක data import කරන්න. මේක `led.json` නමින් save කරලා Data tab එකේ **⋮ → Import JSON** පාවිච්චි කරන්න:

```json
{
  "led": {
    "pattern": 6,
    "speed": 300,
    "custom": "100000:200,010000:200,001000:200,000100:200,000010:200,000001:200"
  }
}
```

### 2. ESP32 firmware

1. [Arduino IDE](https://www.arduino.cc/en/software) install කරන්න.
2. Boards Manager එකෙන් **esp32 by Espressif Systems** board package එක install කරන්න.
3. Library Manager එකෙන් **ArduinoJson (Benoit Blanchon), version 7.x** install කරන්න.
4. `esp32-firmware/esp32-firmware.ino` open කරලා මේවා වෙනස් කරන්න:

```cpp
const char* WIFI_SSID = "YOUR_WIFI";
const char* WIFI_PASS = "YOUR_PASSWORD";
const char* DB_URL    = "https://your-project-default-rtdb.firebasedatabase.app";  // අගට / නැහැ, /led නැහැ
const char* DB_AUTH   = "";   // අවශ්‍ය නම්
```

5. Board එක තෝරලා (උදා: *ESP32 Dev Module*) upload කරන්න.

> **ඇත්ත WiFi password එකයි Firebase URL එකයි GitHub එකට commit කරන්න එපා.** Repository එකේ placeholders ඒවගේම තියලා, ඇත්ත අගයන් දාන්නේ ඔයාගේම computer එකේ විතරයි.

### 3. Web app

1. `web-app/index.html` open කරලා script එකේ උඩින් තියෙන database URL එක දාන්න:

```js
const DB_URL  = 'https://your-project-default-rtdb.firebasedatabase.app';
const DB_AUTH = '';   // අවශ්‍ය නම්
```

2. File එක browser එකකින් open කරන්න, නැත්නම් host කරන්න (පහළින් බලන්න).

## Data ව්‍යුහය

```
led
 ├─ pattern : 0-6
 ├─ speed   : 20-5000   (රටා 1-5 වල step එකකට ms)
 └─ custom  : "100000:200,010000:200,..."   (pattern = 6 නම් පාවිච්චි වෙනවා)
```

### රටා

| අගය | රටාව |
|---|---|
| 0 | Off |
| 1 | සියල්ල blink |
| 2 | Chase (වමේ සිට දකුණට) |
| 3 | Knight Rider (ඉස්සරහට පස්සට) |
| 4 | Alternate (ඔත්තේ / ඉරට්ටේ) |
| 5 | Random |
| 6 | Custom (`custom` string එක පාවිච්චි වෙනවා) |

රටා 1-5 වලට `speed` පාවිච්චි වෙනවා. රටාව 6 `speed` ගණන් ගන්නේ නැහැ, මොකද හැම step එකකම තමන්ගේම කාලය තියෙනවා.

### Custom රටාවේ format එක

```
100000:200,010000:200,001000:500
└─┬──┘ └┬┘
  │     └── ඒ step එක තියෙන කාලය (ms)
  └──────── LED mask එක, 1 = on, 0 = off (pin එකකට එක අකුරක්)
```

- පළවෙනි අකුර `PINS[]` එකේ පළවෙනි pin එක, දෙවැනි අකුර දෙවැනි pin එක, ආදී ලෙස.
- Steps වෙන් කරන්නේ කොමාවෙන්.
- උපරිම step 300ක් සහ අකුරු 3900ක්. මේ සීමාව එන්නේ NVS string එකක ප්‍රමාණයෙන් (bytes 4000ක් විතර). මීට වඩා දිග රටා ඕන නම් ඒවා LittleFS file එකක save කරන්න.
- වැරදි string එකක් ආවොත් ignore කරලා කලින් රටාවම දිගටම දුවනවා.

උදාහරණ:

```
111111:1000,000000:200,111111:200,000000:200     # දිගට පත්තු වෙලා, ඊට පස්සේ වේගෙන් blink
101010:300,010101:300                            # ඔත්තේ/ඉරට්ටේ alternate
```

## වැඩ කරන විදිහ

1. Boot එකේදී ESP32 එක flash එකෙන් අන්තිමට save කරපු රටාව load කරලා ඉක්මනින්ම දුවවනවා.
2. Core 0 එකේ වෙනම task එකක් තත්පර 2කට සැරයක් `/led.json` බලනවා.
3. යමක් වෙනස් වුණොත් task එක අලුත් අගයන් mutex එකක් හරහා main loop එකට භාර දෙනවා.
4. Main loop එක අලුත් රටාව apply කරලා flash එකට ලියනවා, ඒත් දැනට save කරලා තියෙන එකට වඩා වෙනස් නම් විතරයි.

Polling එක (HTTPS handshake එකකට තත්පර 1-2ක් යන්න පුළුවන්) වෙනම core එකක තියෙන නිසා, Firebase බලද්දී animation එක නතර වෙන්නේ නැහැ.

## Web app එක host කරන විදිහ (අවශ්‍ය නම්)

### Firebase Hosting

```bash
npm install -g firebase-tools
firebase login
mkdir led-app && cd led-app
firebase init hosting      # public directory: public, single-page app: No
# web-app/index.html එක public/index.html විදිහට copy කරන්න
firebase deploy --only hosting
```

වෙන විකල්ප: GitHub Pages, Netlify, Cloudflare Pages.

## ආරක්ෂාව ගැන සටහන්

- `index.html` එකේ තියෙන database URL එක, "View source" කළොත් ඕනම කෙනෙකුට පේනවා. UI එකෙන් සැඟවීමෙන් ඒක ආරක්ෂා වෙන්නේ නැහැ.
- Host කරපු page එක public. Link එක තියෙන ඕනම කෙනෙකුට ඔයාගේ LED වෙනස් කරන්න පුළුවන්. ඒක ප්‍රශ්නයක් නම් file එක host නොකර ඔයාගේ device එකේම open කරන්න, නැත්නම් Firebase Authentication දාලා rules වල `auth != null` කියලා කරන්න.
- Firmware එකේ සරල කිරීමට `client.setInsecure()` පාවිච්චි කරලා තියෙනවා, ඒකෙන් TLS certificate verify කරන්නේ නැහැ. Production වලට root CA certificate එක pin කරන්න.

## ගැටළු විසඳීම

| ගැටළුව | විසඳුම |
|---|---|
| `ArduinoJson.h: No such file or directory` | Library Manager එකෙන් ArduinoJson install කරන්න |
| `'JsonDocument' does not name a type` | ඔයාට ArduinoJson 6.x තියෙන්නේ, 7.x වලට update කරන්න |
| Firebase බලද්දී LED තත්පරයක් නතර වෙනවා | Task එකෙන් වැඩ කරන sketch එක (මේ repository එකේ default එක) පාවිච්චි කරන්න |
| LED එකක් නොපත්තු වෙනවා | Mask එකේ ස්ථානය, wiring, සහ ඒ GPIO එක ඔයාගේ board එකේ වෙන දෙයකට පාවිච්චි වෙන්නේ නැද්ද බලන්න (උදා: ESP32-CAM එකේ GPIO 4) |
| Web app එකෙන් කිසිම වෙනසක් වෙන්නේ නැහැ | `DB_URL`, database rules බලන්න, `<ඔයාගේ-url>/led.json` එක browser එකේ open කරලා බලන්න |
| Pattern string එක reject වෙනවා | Format එකයි, අකුරු 3900ට අඩුද කියලත් බලන්න |

## ඉදිරියට කළ හැකි දේවල්

- ඉතා දිග රටා වලට LittleFS storage
- Polling වෙනුවට Firebase streaming (SSE)
- Firebase එකෙන්ම pin list එක වෙනස් කිරීම
- Web app එකට authentication

## License

MIT, නැත්නම් ඔයා කැමති license එකක්. Repository එකට `LICENSE` file එකක් එකතු කරන්න.
