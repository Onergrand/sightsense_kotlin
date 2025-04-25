# SightSense Android Application Installation Manual
## Introduction

SightSense is an Android application that captures visual information through the device camera, processes it using computer vision techniques, and converts it into audio output. This manual will guide you through the installation process so you can work with the project and develop new features.
System Requirements

- Android Studio (latest version recommended)
- JDK 8 or higher
- Android SDK with minimum API level 24 (Android 7.0 Nougat) build.gradle:12-13
- CMake 3.22.1 (for native code compilation) build.gradle:35-36
- Git (for cloning the repository)

### Installation Steps
1. Clone the Repository

```
git clone https://github.com/Onergrand/sightsense_kotlin.git  
cd sightsense_kotlin
```

2. Open the Project in Android Studio

- Launch Android Studio
- Select "Open an Existing Project"
- Navigate to the cloned sightsense_kotlin directory and select it

3. Install Required Dependencies

The project uses several dependencies that should be automatically resolved by Gradle. The main build configuration is defined in the app/build.gradle file. build.gradle:1-4

4. Configure Native Development Kit (NDK)

The application uses native C/C++ code through the Android NDK:

- In Android Studio, go to Tools > SDK Manager
- Select the "SDK Tools" tab
- Check "NDK (Side by side)" and "CMake"
- Click "Apply" and follow the installation instructions

The project is configured to support multiple CPU architectures: build.gradle:22-24

5. Build the Project

- Click on "Build > Make Project" in Android Studio
- Alternatively, you can use the Gradle wrapper from the command line:

```
./gradlew build
```

The Gradle wrapper script is included in the project. gradlew:39-41

6. Run the Application

You can run the application in two ways:

- On an Emulator:
  - Create a virtual device through AVD Manager in Android Studio
  - Select a device with API level 24 or higher
  - Click "Run" in Android Studio
- On a Physical Device:
  - Enable USB debugging on your Android device
  - Connect your device to your computer
  - Allow USB debugging when prompted on your device
  - Select your device in Android Studio and click "Run"

## Project Structure

Understanding the project structure will help you develop new features:
Main Components

- Android/Kotlin Layer: The main application code written in Kotlin
  - MainActivity.kt: The main activity that handles camera operations and UI MainActivity.kt:32-38
- Native Layer: C/C++ code for image processing and audio generation
  - native-lib.cpp: JNI bridge between Kotlin and native code native-lib.cpp:7-12
- Resources: UI layouts, drawables, and other resources
  - Located in the app/src/main/res directory
