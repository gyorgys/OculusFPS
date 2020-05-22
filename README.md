# OculusFPS
Oculus performance debug tool. Launch this exe - it will show a window with Oculus performance stats.

To be able to see these stats in the virtual environment:
- Open Virtual Desktop inside the virtual environment
- Point at the OculusFPS window with your controller, squeeze the trigger and then the grip button.
- Drag the OculusFPS windo into your virtual environment, place it wherewer you want.
- Click at the pushpin icon at the bottom of the window to 'pin' it into the virtual environment.

Note: to build the solution edit the following line in OculusFPS.vcxproj to point to your Oculus SDK directory:

```
<Import Project="D:\SDK\ovr_sdk_win_1.43.0\OVRRootPath.props" />
```
