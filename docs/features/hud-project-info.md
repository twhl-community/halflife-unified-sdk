# Hud Project Info

The hud has a new element called project info.

This element draws project information on the hud:

<a href="https://i.imgur.com/omanjkd.png">
<img src="https://i.imgur.com/omanjkd.png" width="800">
</a>

This hud element is drawn if the `cl_showprojectinfo` cvar is set to `1`. A mod built with `HalfLife_RELEASE_TYPE` set to `Pre-Alpha` or `Alpha` will automatically have this element turned on to make it clear to players that this is a (pre-)alpha build, as well as to more easily capture relevant information in screenshots and videos.

> **Note**
> If you are playing a (pre-)alpha build, please do not turn off this information if you are reporting issues. This information is required to help identify problems.

Detailed information:

| Name | Purpose |
| --- | --- |
| Build type | The CMake build type. One of Debug, MinSizeRel, Release, RelWithDebInfo |
| Release type | The `HalfLife_RELEASE_TYPE` value set by the developer. One of Pre-Alpha, Alpha, Beta, Release |
| Version | The mod version, as defined in the CMake `project` call in the root CMakeLists.txt file |
| Branch | The Git branch that this build was made from |
| Tag | The last Git tag in this Git branch that the build was made from. If there are no tags this will read `None`, if the build was made by Continuous Integration it will read `CI build` |
| Commit Hash | The Git commit hash that the build was made from. If the Git repository had uncommitted changes the text `-dirty` will be appended |

Information pertaining to the client and server is provided for both.

* If the client is a (pre-)alpha build an additional line will read: `WIP build not suited for use (testing only)`
* If the client and server versions do not match an additional line will read: `Warning: Client and server versions do not match`
* If the client and server commit hashes do not match an additional line will read: `Warning: Client and server builds do not match`
