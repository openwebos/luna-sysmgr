/**
 *  Copyright (c) 2012 Hewlett-Packard Development Company, L.P.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef _NyxSensorCommonTypes_h
#define _NyxSensorCommonTypes_h

/** Sensor Names */
class SensorNames {
public:
    static const char* strAccelerometer()               { return "accelerometer"; }
    static const char* strOrientation()                 { return "orientation"; }
    static const char* strShake()                       { return "shake"; }
    static const char* strALS()                         { return "als"; }
    static const char* strAngularVelocity()             { return "angularVelocity"; }
    static const char* strBearing()                     { return "bearing"; }
    static const char* strGravity()                     { return "gravity"; }
    static const char* strLinearAcceleration()          { return "linearAcceleration"; }
    static const char* strMagneticField()               { return "magneticField"; }
    static const char* strScreenProximity()             { return "screenProximity"; }
    static const char* strRotation()                    { return "rotation"; }
    static const char* strLogicalDeviceMotion()         { return "logicalDeviceMotion"; }
    static const char* strLogicalDeviceOrientation()    { return "logicalDeviceOrientation"; }
};

/**
 * NYX Json Strings
 */
class NYXJsonStringConst {
public:
    static const char* strW()                       { return "w"; }
    static const char* strX()                       { return "x"; }
    static const char* strY()                       { return "y"; }
    static const char* strZ()                       { return "z"; }
    static const char* strRawX()                    { return "rowX"; }
    static const char* strRawY()                    { return "rowY"; }
    static const char* strRawZ()                    { return "rowZ"; }
    static const char* strWorldX()                  { return "worldX"; }
    static const char* strWorldY()                  { return "worldY"; }
    static const char* strWorldZ()                  { return "worldX"; }
    static const char* strPosition()                { return "position"; }
    static const char* strShakeState()              { return "shakeState"; }
    static const char* strShakeMagnitude()          { return "shakeMagnitude"; }
    static const char* strShakeStart()              { return "Start"; }
    static const char* strShaking()                 { return "Shaking"; }
    static const char* strShakeEnd()                { return "End"; }
    static const char* strRotationMatrix()          { return "rotationMatrix"; }
    static const char* strQuaternionVector()        { return "quaternionVector"; }
    static const char* strEulerAngle()              { return "eulerAngle"; }
    static const char* strRoll()                    { return "roll"; }
    static const char* strPitch()                   { return "pitch"; }
    static const char* strYaw()                     { return "yaw"; }
    static const char* strLightIntensity()          { return "lightIntensity"; }
    static const char* strMagnetic()                { return "magnetic"; }
    static const char* strTrueBearing()             { return "trueBearing"; }
    static const char* strConfidence()              { return "confidence"; }
    static const char* strOrientationFaceUp()       { return "Face Up"; }
    static const char* strOrientationFaceDown()     { return "Face Down"; }
    static const char* strOrientationFaceForward()  { return "Face Forward"; }
    static const char* strOrientationFaceBack()     { return "Face Back"; }
    static const char* strOrientationLeft()         { return "Left"; }
    static const char* strOrientationRight()        { return "Right"; }
    static const char* strEmpty()                   { return ""; }
};


#endif // _NyxSensorCommonTypes_h
