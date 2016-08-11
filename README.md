# Sigfox GPS Tracker
Tutorial on how to make a GPS tracker using the TD1205 with Sigfox connectivity

### Version
0.0.1

## Getting Started

### Getting the sources
The SDK source code and examples are no longer distributed with the TD RF Module SDK Tools zip file but
are available through the <https://github.com/Telecom-Design/TD_RF_Module_SDK> Github repository
once you register your Telecom Design Evaluation Board (EVB) by following the steps described on
<http://rfmodules.td-next.com/sdk/>.

The following steps detail how to download the source code and import all projects into the Eclipse
environment.

  1. Navigate to the `"C:\TD\TD_RF_Module_SDK-v6.0.0\eclipse"` folder and double-click on the
     `"eclipse.exe"` icon
  2. Open the `"File"` menu and select the `"Import..."` item.
  3. In the `"Import"` dialog, unfold the `"Git"` folder by clicking on the `"+"` sign left to it,
     select the `"Projects from Git"` item and click on the `"Next >"` button
  4. In the `"Import Projects from Git"`, select the `"URI"` icon ad click on the `"Next >"` button
  5. Enter The Github repository URL `"https://github.com/Telecom-Design/TD_RF_Module_SDK.git"`
     in the `"URI"` field
  6. Enter your Github username and password in the `"User:"` and `"Password:"` fields,
     respectively and click on the `"Next >"` button
  7. Check the `"Master"` branch box and click on the `"Next >"` button
  8. Enter `"C:\TD\TD_RF_Module_SDK-v6.0.0\Github\TD_RF_Module_SDK"` in the `"Directory:"` field and
     click on the `"Next >"` button
  9. Check the `"Import existing projects"` radio button and click on the `"Next >"` button
  10. Click on the `"Finish"` button. The Git import will take place, this may take a while

All the available libraries and examples should now be available in the Project Explorer panel.

### Organizing the Sources

By default, all projects are presented in Eclipse at the same level without particular organization
except that they are sorted alphabetically.

In order to have a more logical organization, we must import an Eclipse "Working Set" that will
provide a grouping of projects by categories.

To do so, launch Eclipse by navigating to the `"C:\TD\TD_RF_Module_SDK-v6.0.0\eclipse"` folder and
double-click on the `"eclipse.exe"` icon (if not already done) and:

  1. Open the `"File"` menu and select the `"Import..."` item.
  2. In the `"Import"` dialog, unfold the `"General"` folder by clicking on the `"+"` sign left to
     it, select the `"Working Sets"` item and click on the `"Next >"` button
  3. Enter `"C:\TD\TD_RF_Module_SDK-v6.0.0\Github\TD_RF_Module_SDK\TD_RF_Module_SDK.wst"` in the
     `"Browse..."` field, check all working sets and click on the `"Finish"` button
  4. Click on the small downwards arrow in the top-right corner of the `"Project Explorer"` panel
     and select the `"Select Working Sets..."` item
  5. In the `"Select Working Sets"` dialog, click on the `"Select All"`, then on the `"OK"` button
  6. Click on the small downwards arrow in the top-right corner of the `"Project Explorer"` panel
     again and select the `"Top Level Elements > Working Sets"` item

All the available libraries and examples should now be better organized in the Project Explorer panel.

### Import the `"Sigfox GPS Tracker"` project

Open the `"File"` menu and select the `"Import..."` item.
  3. In the `"Import"` dialog, unfold the `"Git"` folder by clicking on the `"+"` sign left to it,
     select the `"Projects from Git"` item and click on the `"Next >"` button
  4. In the `"Import Projects from Git"`, select the `"URI"` icon ad click on the `"Next >"` button
  5. Enter The Github repository URL `"https://github.com/luisomoreau/sigfox_gps_tracker.git"`
     in the `"URI"` field
  6. Enter your Github username and password in the `"User:"` and `"Password:"` fields,
     respectively and click on the `"Next >"` button
  7. Check the `"Master"` branch box and click on the `"Next >"` button
  8. Enter `"C:\TD\TD_RF_Module_SDK-v6.0.0\workspace"` in the `"Directory:"` field and
     click on the `"Next >"` button
  9. Check the `"Import existing projects"` radio button and click on the `"Next >"` button
  10. Click on the `"Finish"` button.
  11. You can also create a workspace called `"Tutorial"`and add the project to this workspace.

### Add the environment variables

With the exception of the binary-only static libraries in the `"libtddrivers"` and `"libtdrf"`
projects, all deliverables are presented in source form only, and must be compiled to obtain an
executable firmware. To compile properly you must add the gcc path into your Eclipse environment variables
by opening: Window -> Preferences -> C/C++ -> Environment -> Add...

Please add the PATH variable with the following value:

C:\TD\TD_RF_Module_SDK-v6.0.0\gnu\bin

### Compiling for TD1205P
The TD1205P have not been officially released yet, you should be able to compile the project for the TD1205P with this project (configuration is included in this repository).
However, if you wish to go back from scratch, few tricks will be needed.

Then if we take this project as an example, here are the steps required to compile it:

  1. In order to avoid unnecessary rebuilds of the common libraries, it is best to set the right
     build configuration for all these libraries: unfold the `"Common_Libraries"` working set in the
     Project Explorer panel on the left side, then select all the projects by clicking on the first one
     in the list, pressing the `"SHIFT"`key, then clicking on the last one in the list
  2. Click on the small downwards arrow right next to the `"Hammer"` icon in the top menu bar and
     select the right build configuration corresponding to your board: `"TD1205P"` `"GCC Release EZR"` for a stripped down firmware
  3. Unfold the `"Tutorial"` working set in the Project Explorer panel on the left side, then
     select the `"sigfox_gps_tracker"` project
  4. Click right on the project and select `"Properties"`.
  5. Go to C/C++ build -> Build variable. Make sure that the TD1205P appear. If not you can create it by clicking on `"Manage Configuration"` -> `"New"` -> add `"TD1205P Release"` and copy the `"TD1508"` configuration.
  6. Then go to `"Settings"`->`"Preprocessor"` and replace the revision with `"MODULE_REVISION=REVISION_TD1205P"`. Make sure you do it for both preprocessors tab (under assembler and compiler)
  7. Click on OK
  8. Open the file `"main.c"`
  4. Click again on the small downwards arrow right next to the `"Hammer"` icon in the top menu bar and
     select the same build configuration as for the libraries above (`"TD1205P"`)
  5. Compilation of the `"sigfox_gps_tracker"`project and all the required dependencies will take place, which can
     be monitored in the `"Console"` tab of the bottom panel.

The same procedure can be used for all the example projects when required.

### Flash project
Unlike some dedicated embedded Interactive Development Environments (IDEs), Eclipse does not come with
fixed Flash/Debug commands or menu buttons: they need to be added explicitly on a project by project basis.

Fortunately, these Flash/Debug `"Launchers"` can be imported or duplicated to other projects easily:

  1. Open the `"File"` menu and select the `"Import..."` item.
  2. In the `"Import"` dialog, unfold the `"Run/Debug"` folder by clicking on the `"+"` sign left to it,
     select the `"Launch Configurations"` item and click on the `"Next >"` button
  3. Enter `"C:/TD/TD_RF_Module_SDK-v6.0.0/Github/TD_RF_Module_SDK/Eclipse Launchers"` in the `"Browse..."`
     field, check the box in front of the `"Eclipse Launchers"` item and click on the `"Finish"` button
  4. All the default Flash/Debug `"Launchers"` will be added to the top menu `"Bug"` and `"Green Circle
     with With Right Arrow and Briefcase"` icons
  5. If required, you can edit these `"Launchers"` by choosing the "Debug Configurations..."` or
     `"External Tools Configurations..." entries in the menu obtained by the small downwards arrow right
     next to the corresponding `"Bug"` or `"Green Circle with With Right Arrow and Briefcase"` icon
  6. In the dialog windows that opens, you can edit a given configuration directly and click on the
     `"Apply"` and `"Close"` buttons, or you an duplicate it by right-clicking on it and selecting the
     `"Duplicate"` entry in the contextual menu that pops up

Then, to flash a firmware to the TD12xx/TD15xx board:

  1. Select the desired project in the Project Explorer panel on the left side
  2. Right-click on the project and select the `"Build Configurations > Set Active >"` and the desired
     build configuration as explained above
  3. Optionally click on the `"Hammer"` icon in the top menu bar to build the project
  4. Click on the  the small downwards arrow right next to the `"Green Circle with With Right Arrow and
     Briefcase"` icon and select `"Flash Selected Project"` in the contextual menu.

### Debug
You can debug the project using Putty and enable DEBUG variables in the project.