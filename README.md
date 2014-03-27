cypress-fx3-sdk-linux
=====================

##1. Install Eclipse & Eclipse C/C++ Development Tools
*$ sudo apt-get install eclipse eclipse-cdt*

##2. Install GNU ARM Eclipse Plug-in
The recommended way to install this plug-in is to use the Eclipse standard install/update mechanism:  
1. Start Eclipse (from Terminal):  
*$ eclipse &*  
2. Eclipse menu: Help->Install New Software  
3. In the Install window, click the "Add..." button (for future updates, you will be able to select the URL from the dropdown)  
4. Fill in Location: *http://gnuarmeclipse.sourceforge.net/updates*  
5. Click the "OK" button  
6. Normally the main window should list a group named "CDT GNU Cross Development Tools" - expand it.  
7. Select "GNU ARM C/C++ Development Support"  
8. Click the Next button  
9. Accept the unsigned plug-in and install  

Note: As we saved the update site URL in Step 3, further updates are greatly simplified via: Help->Check For Updates

##3. Setup Eclipse
1. Add the following lines to the end of ~/.bashrc using a text editor (so Eclipse knows where our project and toolchain will be located):  
*export FX3_INSTALL_PATH=$HOME/workspace/cypress-fx3-sdk-linux*  
*export PATH="$PATH:$FX3_INSTALL_PATH/util/arm-2011.03/bin"*  
*export ARMGCC_INSTALL_PATH=$FX3_INSTALL_PATH/util/arm-2011.03*  
*export ARMGCC_VERSION=4.5.2*  
2. Start Eclipse (from a fresh instance of Terminal):  
*$ eclipse &*
3. Use the default workspace path when prompted (i.e. ~/workspace)
4. Clone this repository to the Eclipse workspace folder:  
*$ cd ~/workspace*  
*$ git clone https://github.com/nickdademo/cypress-fx3-sdk-linux.git*  
5. Compile the included elf2img tool:  
*$ cd ~/workspace/cypress-fx3-sdk-linux/util/elf2img*  
*$ gcc elf2img.c -o elf2img -Wall*  
6. Import the projects you require into Eclipse: _File->Import->General->Existing Project into Workspace_ - select _cypress-fx3-sdk-linux/firmware_ as the root directory.  
Note 1: Ensure you DO NOT import the *cyu3lpp* project.  
Note 2: Import *CyStorBootWriter* if you will be writing firmware to FX3S Storage Port 0.  
7. Also import the following TWO (2) projects located in the "FX3_SDK_1_3_1_SRC" folder in the top-level directory:  
*boot_fw*  
*sdk*  
8. Build the entire project workspace: Project->Build All

##4. Flashing
###4A. Flashing the Device (using CyUSB Suite for Linux)
1. Install dependencies:  
*$ sudo apt-get install libqt4-dev qt4-qmake libusb-1.0-0-dev*  
2. Download _EZ-USB FX3 SDK v1.3.1 for Linux_ at the following link (Cypress login required):  
http://www.cypress.com/?docID=42387&dlm=1  
3. Open the downloaded SDK archive and extract 'cyusb_linux_1.0.4.tar.gz' to your Home folder.
4. Compile application:  
*$ cd ~/cyusb_linux_1.0.4/*  
*$ sudo ./install.sh*  
5. Create symbolic link to application binary:  
*$ sudo ln -s bin/cyusb_linux /usr/bin/cyusb_linux*
6. Restart system
7. Run _CyUSB Suite for Linux_ with the following command in Terminal:  
*$ cyusb_linux*

###4B. Flashing the Device (using Cypress command line tools through Eclipse)
1. Install dependencies:  
*$ sudo apt-get install libusb-1.0-0-dev*  
2. Compile *cyusb_linux_1.0.4* tool:  
*$ cd ~/workspace/cypress-fx3-sdk-linux/util/cyusb_linux_1.0.4_cmd/*  
*$ make*  
*$ cd src/*  
*$ make*
3. Compile *cyfwstorprog* tool (for flashing to SD/eMMC):  
*$ cd ~/workspace/cypress-fx3-sdk-linux/util/cyfwstorprog_linux/*  
*$ make*  
4. Make *cyfwstorprog* tool run WITHOUT a _sudo_ password prompt in Eclipse:  
*$ sudo gedit /etc/sudoers*  
Add the following to the end of the _sudoers_ file (replace YOUR_USERNAME with your own and ensure the path to the tool is correct):  
*YOUR_USERNAME ALL = (ALL) ALL*  
*YOUR_USERNAME ALL = (root) NOPASSWD: /home/YOUR_USERNAME/workspace/cypress-fx3-sdk-linux/util/cyfwstorprog_linux/cyfwstorprog*  
5. Create a .launches directory in the Eclipse workspace:  
*$ mkdir -p ~/workspace/.metadata/.plugins/org.eclipse.debug.core/.launches*  
6. Copy Eclipse launch files to appropriate folder:  
*$ cp ~/workspace/cypress-fx3-sdk-linux/util/cyusb_linux_1.0.4_cmd/eclipse_launches/*_\*.launch ~/workspace/.metadata/.plugins/org.eclipse.debug.core/.launches/_    
7. Start Eclipse and program the FX3 using the External Tools menu! (Note: You may have to add the launches as favourites via "Organize Favourites" to make them visible).

**IMPORTANT:** The **CyStorBootWriter** project must be compiled and flashed to the FX3S RAM before attempting to use the  _cyfwstorprog_ tool to write firmware to the SD/eMMC.

The source for the  _cyfwstorprog_ tool can be found at https://github.com/nickdademo/cyfwstorprog_linux

##Notes
### Eclipse Build Configurations
Fx3BootAppGcc:   
**Debug:** Optimization level = OPTIMIZE MOST (O3)  
**Release:** Optimization level = OPTIMIZE MORE (O2)

boot_fw:  
**Default:** Optimization level = OPTIMIZE (O1)

sdk:   
**Debug:** Optimization level = NONE (O0)  
**Release:** Optimization level = OPTIMIZE (O1)

_All other projects:_   
**Debug:** Optimization level = NONE (O0)  
**Release:** Optimization level = OPTIMIZE MORE (O2)

### Project Dependencies
In order for required libraries (.a files) to exist when they are required during building of certain projects, the build order of Eclipse projects is set via the "Project References" project option:  
**Fx3BootAppGcc:** References "boot_fw"  
**boot_fw:** No references  
**sdk:** No references  
**_All other projects:_** References "sdk"
