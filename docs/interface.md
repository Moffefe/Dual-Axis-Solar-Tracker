# Web panel

The ESP8266 serves a Metro UI interface with a log reader and a date/time form. These browser captures use the HTML, scripts, styles and icon font shipped with the project. A simulated SD directory supplies the records for the example below. [Original web assets](../libraries/WiFiModuleESP/Assets) · [Project overview](../README.md)

## Read Logs

![Desktop diagnostic panel with a one-minute free-heap trace and four labelled state transitions](../media/read-logs-desktop.png)

The graph covers **10:24–10:25 on 18 November 2026**. Its horizontal axis is local device time and its vertical axis is free ESP8266 heap in bytes. Thirty simulated samples, spaced two seconds apart, range from **31,144 to 31,600 bytes**. The firmware alternates its free-heap and loop-duration channels. The example selects the free-heap channel. [ESP8266 logger](../libraries/WiFiModuleESP/WiFiModuleESP.cpp) · [Session data](example-session.json)

The timeline is zoomed around the correction. The firmware’s original Polish labels identify the state and its initialization word.

| Device time | State | Interpretation |
| --- | --- | --- |
| 10:24:03 | `idle` | The motors are off and the controller is sampling light. |
| 10:24:10 | `sunAware` | The light difference exceeds the threshold. The confirmation delay starts. |
| 10:24:20 | `followSun` | The direction has held for ten seconds. The controller starts the selected axes. |
| 10:24:27 | `idle` | The correction has ended and the controller resumes monitoring. |

The graph and timeline were drawn by the original log client using the event definitions bundled in the page. [Read Logs markup and event definitions](../libraries/WiFiModuleESP/Assets/ReadLogsBody.html) · [Log decoder](../libraries/WiFiModuleESP/Assets/LoggerClient/LoggerClient.js)

## Set Time on a phone

<img src="../media/set-time-mobile.png" width="350" alt="Phone-width Set Time panel with 18 November 2026 and 10:25 selected">

The date and time controls are set to the end of the sample interval. On the device, the **Set** button sends the selected values to the ESP8266 time-setting handler. [Set Time form](../libraries/WiFiModuleESP/Assets/SetTimeBody.html)

The navigation also includes Home and Storage, which are placeholder views in this source tree.
