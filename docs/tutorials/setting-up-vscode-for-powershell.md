# Setting up Visual Studio Code to edit PowerShell scripts

This guide will help you set up Visual Studio Code to edit and debug PowerShell scripts.

# Install Visual Studio Code

First, install Visual Studio Code. You can download it from this website: https://code.visualstudio.com/

# Install the PowerShell extension

Install the VS Code PowerShell extension. See this guide for more information: https://code.visualstudio.com/docs/languages/powershell

# Configure the PowerShell extension

Open VS Code. Go to `File->Preferences->Settings`. In the Settings dialog, go to `Extensions->PowerShell Configuration` and enable the setting `Debugging: Create Temporary Integrated Console`. This will make VS Code create a new PowerShell instance every time you run scripts through the editor. This helps to avoid "bleeding" script state from previous executions into the current execution.

Also set the `Cwd` setting to `${fileDirname}`. This will make VS Code set the working directory for scripts to the directory that contains the script. The scripts used by the Unified SDK depend on their location relative to the mod directory, so this is very important.

# Open the scripts directory

Go to `File->Open Folder...` and open the `scripts` directory in the mod directory. You will now be able to edit all of the PowerShell scripts by opening the scripts through VS Code.

# Configure launch options

With the `scripts` directory open in VS Code, open the `Run and Debug` tab using the menu on the left side of the window. Click the `create a launch.json file` button to create a default configuration file. VS Code will now open a drop-down menu. Select the option `Launch Current File`.

VS Code will open the newly created configuration file. It should look something like this:
```jsonc
{
    // Use IntelliSense to learn about possible attributes.
    // Hover to view descriptions of existing attributes.
    // For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387
    "version": "0.2.0",
    "configurations": [
        {
            "name": "PowerShell: Launch Current File",
            "type": "PowerShell",
            "request": "launch",
            "script": "${file}",
            "cwd": "${file}"
        }
    ]
}
```

Modify it by adding a new key:
```jsonc
{
    // Use IntelliSense to learn about possible attributes.
    // Hover to view descriptions of existing attributes.
    // For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387
    "version": "0.2.0",
    "configurations": [
        {
            "name": "PowerShell: Launch Current File",
            "type": "PowerShell",
            "request": "launch",
            "script": "${file}",
            "cwd": "${file}",
            "args": [
                "-Verbose"
            ]
        }
    ]
}
```

The addition of the `-Verbose` argument will make the scripts output more information which is useful when debugging them.

# Run scripts

You can now run the currently active script by going back to the `Run and Debug` tab and clicking the green triangle next to the `PowerShell: Launch Current File` option. You can also run it by pressing F5.

For more information on running and debugging PowerShell scripts you should consult the official documentation for VS Code and PowerShell on Microsoft's website.
