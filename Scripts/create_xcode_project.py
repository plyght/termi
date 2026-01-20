#!/usr/bin/env python3

import os
import uuid
import plistlib
from pathlib import Path

project_root = Path(__file__).parent.parent
project_name = "termi"
bundle_id = "com.termi.app"

pbxproj_uuid = lambda: str(uuid.uuid4()).replace("-", "")[:24].upper()

app_files = {
    "TermiApp.swift": pbxproj_uuid(),
    "ContentView.swift": pbxproj_uuid(),
    "TerminalView.swift": pbxproj_uuid(),
}

manager_files = {
    "EmulatorManager.swift": pbxproj_uuid(),
}

bridge_files = {
    "EmulatorBridge.swift": pbxproj_uuid(),
    "FilesystemBridge.swift": pbxproj_uuid(),
    "termi-Bridging-Header.h": pbxproj_uuid(),
}

resources = {
    "Info.plist": pbxproj_uuid(),
    "termi.entitlements": pbxproj_uuid(),
}

libraries = {
    "../Kernel/syscall/libsyscall.a": pbxproj_uuid(),
    "../Filesystem/fakefs/libfakefs.a": pbxproj_uuid(),
}

main_group_id = pbxproj_uuid()
app_group_id = pbxproj_uuid()
ui_group_id = pbxproj_uuid()
managers_group_id = pbxproj_uuid()
bridge_group_id = pbxproj_uuid()
resources_group_id = pbxproj_uuid()
frameworks_group_id = pbxproj_uuid()

native_target_id = pbxproj_uuid()
build_config_list_target = pbxproj_uuid()
build_config_debug_target = pbxproj_uuid()
build_config_release_target = pbxproj_uuid()
build_config_list_project = pbxproj_uuid()
build_config_debug_project = pbxproj_uuid()
build_config_release_project = pbxproj_uuid()

sources_build_phase_id = pbxproj_uuid()
frameworks_build_phase_id = pbxproj_uuid()
resources_build_phase_id = pbxproj_uuid()

project_obj_id = pbxproj_uuid()

pbxproj_content = f"""// !$*UTF8*$!
{{
	archiveVersion = 1;
	classes = {{
	}};
	objectVersion = 56;
	objects = {{

/* Begin PBXBuildFile section */
		{app_files["TermiApp.swift"]}01 /* TermiApp.swift in Sources */ = {{isa = PBXBuildFile; fileRef = {app_files["TermiApp.swift"]} /* TermiApp.swift */; }};
		{app_files["ContentView.swift"]}01 /* ContentView.swift in Sources */ = {{isa = PBXBuildFile; fileRef = {app_files["ContentView.swift"]} /* ContentView.swift */; }};
		{app_files["TerminalView.swift"]}01 /* TerminalView.swift in Sources */ = {{isa = PBXBuildFile; fileRef = {app_files["TerminalView.swift"]} /* TerminalView.swift */; }};
		{manager_files["EmulatorManager.swift"]}01 /* EmulatorManager.swift in Sources */ = {{isa = PBXBuildFile; fileRef = {manager_files["EmulatorManager.swift"]} /* EmulatorManager.swift */; }};
		{bridge_files["EmulatorBridge.swift"]}01 /* EmulatorBridge.swift in Sources */ = {{isa = PBXBuildFile; fileRef = {bridge_files["EmulatorBridge.swift"]} /* EmulatorBridge.swift */; }};
		{bridge_files["FilesystemBridge.swift"]}01 /* FilesystemBridge.swift in Sources */ = {{isa = PBXBuildFile; fileRef = {bridge_files["FilesystemBridge.swift"]} /* FilesystemBridge.swift */; }};
		{libraries["../Kernel/syscall/libsyscall.a"]}01 /* libsyscall.a in Frameworks */ = {{isa = PBXBuildFile; fileRef = {libraries["../Kernel/syscall/libsyscall.a"]} /* libsyscall.a */; }};
/* End PBXBuildFile section */

/* Begin PBXFileReference section */
		{app_files["TermiApp.swift"]} /* TermiApp.swift */ = {{isa = PBXFileReference; lastKnownFileType = sourcecode.swift; path = TermiApp.swift; sourceTree = "<group>"; }};
		{app_files["ContentView.swift"]} /* ContentView.swift */ = {{isa = PBXFileReference; lastKnownFileType = sourcecode.swift; path = ContentView.swift; sourceTree = "<group>"; }};
		{app_files["TerminalView.swift"]} /* TerminalView.swift */ = {{isa = PBXFileReference; lastKnownFileType = sourcecode.swift; path = TerminalView.swift; sourceTree = "<group>"; }};
		{manager_files["EmulatorManager.swift"]} /* EmulatorManager.swift */ = {{isa = PBXFileReference; lastKnownFileType = sourcecode.swift; path = EmulatorManager.swift; sourceTree = "<group>"; }};
		{bridge_files["EmulatorBridge.swift"]} /* EmulatorBridge.swift */ = {{isa = PBXFileReference; lastKnownFileType = sourcecode.swift; path = EmulatorBridge.swift; sourceTree = "<group>"; }};
		{bridge_files["FilesystemBridge.swift"]} /* FilesystemBridge.swift */ = {{isa = PBXFileReference; lastKnownFileType = sourcecode.swift; path = FilesystemBridge.swift; sourceTree = "<group>"; }};
		{bridge_files["termi-Bridging-Header.h"]} /* termi-Bridging-Header.h */ = {{isa = PBXFileReference; lastKnownFileType = sourcecode.c.h; path = "termi-Bridging-Header.h"; sourceTree = "<group>"; }};
		{resources["Info.plist"]} /* Info.plist */ = {{isa = PBXFileReference; lastKnownFileType = text.plist.xml; path = Info.plist; sourceTree = "<group>"; }};
		{resources["termi.entitlements"]} /* termi.entitlements */ = {{isa = PBXFileReference; lastKnownFileType = text.plist.entitlements; path = termi.entitlements; sourceTree = "<group>"; }};
		{libraries["../Kernel/syscall/libsyscall.a"]} /* libsyscall.a */ = {{isa = PBXFileReference; lastKnownFileType = archive.ar; name = libsyscall.a; path = ../Kernel/syscall/libsyscall.a; sourceTree = "<group>"; }};
		{native_target_id}02 /* termi.app */ = {{isa = PBXFileReference; explicitFileType = wrapper.application; includeInIndex = 0; path = termi.app; sourceTree = BUILT_PRODUCTS_DIR; }};
/* End PBXFileReference section */

/* Begin PBXFrameworksBuildPhase section */
		{frameworks_build_phase_id} /* Frameworks */ = {{
			isa = PBXFrameworksBuildPhase;
			buildActionMask = 2147483647;
			files = (
				{libraries["../Kernel/syscall/libsyscall.a"]}01 /* libsyscall.a in Frameworks */,
			);
			runOnlyForDeploymentPostprocessing = 0;
		}};
/* End PBXFrameworksBuildPhase section */

/* Begin PBXGroup section */
		{main_group_id} = {{
			isa = PBXGroup;
			children = (
				{app_group_id} /* App */,
				{bridge_group_id} /* Bridge */,
				{resources_group_id} /* Resources */,
				{frameworks_group_id} /* Frameworks */,
				{native_target_id}03 /* Products */,
			);
			sourceTree = "<group>";
		}};
		{app_group_id} /* App */ = {{
			isa = PBXGroup;
			children = (
				{app_files["TermiApp.swift"]} /* TermiApp.swift */,
				{ui_group_id} /* UI */,
				{managers_group_id} /* Managers */,
			);
			path = App;
			sourceTree = "<group>";
		}};
		{ui_group_id} /* UI */ = {{
			isa = PBXGroup;
			children = (
				{app_files["ContentView.swift"]} /* ContentView.swift */,
				{app_files["TerminalView.swift"]} /* TerminalView.swift */,
			);
			path = UI;
			sourceTree = "<group>";
		}};
		{managers_group_id} /* Managers */ = {{
			isa = PBXGroup;
			children = (
				{manager_files["EmulatorManager.swift"]} /* EmulatorManager.swift */,
			);
			path = Managers;
			sourceTree = "<group>";
		}};
		{bridge_group_id} /* Bridge */ = {{
			isa = PBXGroup;
			children = (
				{bridge_files["EmulatorBridge.swift"]} /* EmulatorBridge.swift */,
				{bridge_files["FilesystemBridge.swift"]} /* FilesystemBridge.swift */,
				{bridge_files["termi-Bridging-Header.h"]} /* termi-Bridging-Header.h */,
			);
			path = Bridge;
			sourceTree = "<group>";
		}};
		{resources_group_id} /* Resources */ = {{
			isa = PBXGroup;
			children = (
				{resources["Info.plist"]} /* Info.plist */,
				{resources["termi.entitlements"]} /* termi.entitlements */,
			);
			path = Resources;
			sourceTree = "<group>";
		}};
		{frameworks_group_id} /* Frameworks */ = {{
			isa = PBXGroup;
			children = (
				{libraries["../Kernel/syscall/libsyscall.a"]} /* libsyscall.a */,
			);
			name = Frameworks;
			sourceTree = "<group>";
		}};
		{native_target_id}03 /* Products */ = {{
			isa = PBXGroup;
			children = (
				{native_target_id}02 /* termi.app */,
			);
			name = Products;
			sourceTree = "<group>";
		}};
/* End PBXGroup section */

/* Begin PBXNativeTarget section */
		{native_target_id} /* termi */ = {{
			isa = PBXNativeTarget;
			buildConfigurationList = {build_config_list_target} /* Build configuration list for PBXNativeTarget "termi" */;
			buildPhases = (
				{sources_build_phase_id} /* Sources */,
				{frameworks_build_phase_id} /* Frameworks */,
				{resources_build_phase_id} /* Resources */,
			);
			buildRules = (
			);
			dependencies = (
			);
			name = termi;
			productName = termi;
			productReference = {native_target_id}02 /* termi.app */;
			productType = "com.apple.product-type.application";
		}};
/* End PBXNativeTarget section */

/* Begin PBXProject section */
		{project_obj_id} /* Project object */ = {{
			isa = PBXProject;
			attributes = {{
				BuildIndependentTargetsInParallel = 1;
				LastSwiftUpdateCheck = 2600;
				LastUpgradeCheck = 2600;
				TargetAttributes = {{
					{native_target_id} = {{
						CreatedOnToolsVersion = 26.0;
					}};
				}};
			}};
			buildConfigurationList = {build_config_list_project} /* Build configuration list for PBXProject "termi" */;
			compatibilityVersion = "Xcode 14.0";
			developmentRegion = en;
			hasScannedForEncodings = 0;
			knownRegions = (
				en,
				Base,
			);
			mainGroup = {main_group_id};
			productRefGroup = {native_target_id}03 /* Products */;
			projectDirPath = "";
			projectRoot = "";
			targets = (
				{native_target_id} /* termi */,
			);
		}};
/* End PBXProject section */

/* Begin PBXResourcesBuildPhase section */
		{resources_build_phase_id} /* Resources */ = {{
			isa = PBXResourcesBuildPhase;
			buildActionMask = 2147483647;
			files = (
			);
			runOnlyForDeploymentPostprocessing = 0;
		}};
/* End PBXResourcesBuildPhase section */

/* Begin PBXSourcesBuildPhase section */
		{sources_build_phase_id} /* Sources */ = {{
			isa = PBXSourcesBuildPhase;
			buildActionMask = 2147483647;
			files = (
				{app_files["TermiApp.swift"]}01 /* TermiApp.swift in Sources */,
				{app_files["ContentView.swift"]}01 /* ContentView.swift in Sources */,
				{app_files["TerminalView.swift"]}01 /* TerminalView.swift in Sources */,
				{manager_files["EmulatorManager.swift"]}01 /* EmulatorManager.swift in Sources */,
				{bridge_files["EmulatorBridge.swift"]}01 /* EmulatorBridge.swift in Sources */,
				{bridge_files["FilesystemBridge.swift"]}01 /* FilesystemBridge.swift in Sources */,
			);
			runOnlyForDeploymentPostprocessing = 0;
		}};
/* End PBXSourcesBuildPhase section */

/* Begin XCBuildConfiguration section */
		{build_config_debug_project} /* Debug */ = {{
			isa = XCBuildConfiguration;
			buildSettings = {{
				ALWAYS_SEARCH_USER_PATHS = NO;
				ASSETCATALOG_COMPILER_GENERATE_SWIFT_ASSET_SYMBOL_EXTENSIONS = YES;
				CLANG_ANALYZER_NONNULL = YES;
				CLANG_ANALYZER_NUMBER_OBJECT_CONVERSION = YES_AGGRESSIVE;
				CLANG_CXX_LANGUAGE_STANDARD = "gnu++20";
				CLANG_ENABLE_MODULES = YES;
				CLANG_ENABLE_OBJC_ARC = YES;
				CLANG_ENABLE_OBJC_WEAK = YES;
				CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING = YES;
				CLANG_WARN_BOOL_CONVERSION = YES;
				CLANG_WARN_COMMA = YES;
				CLANG_WARN_CONSTANT_CONVERSION = YES;
				CLANG_WARN_DEPRECATED_OBJC_IMPLEMENTATIONS = YES;
				CLANG_WARN_DIRECT_OBJC_ISA_USAGE = YES_ERROR;
				CLANG_WARN_DOCUMENTATION_COMMENTS = YES;
				CLANG_WARN_EMPTY_BODY = YES;
				CLANG_WARN_ENUM_CONVERSION = YES;
				CLANG_WARN_INFINITE_RECURSION = YES;
				CLANG_WARN_INT_CONVERSION = YES;
				CLANG_WARN_NON_LITERAL_NULL_CONVERSION = YES;
				CLANG_WARN_OBJC_IMPLICIT_RETAIN_SELF = YES;
				CLANG_WARN_OBJC_LITERAL_CONVERSION = YES;
				CLANG_WARN_OBJC_ROOT_CLASS = YES_ERROR;
				CLANG_WARN_QUOTED_INCLUDE_IN_FRAMEWORK_HEADER = YES;
				CLANG_WARN_RANGE_LOOP_ANALYSIS = YES;
				CLANG_WARN_STRICT_PROTOTYPES = YES;
				CLANG_WARN_SUSPICIOUS_MOVE = YES;
				CLANG_WARN_UNGUARDED_AVAILABILITY = YES_AGGRESSIVE;
				CLANG_WARN_UNREACHABLE_CODE = YES;
				CLANG_WARN__DUPLICATE_METHOD_MATCH = YES;
				COPY_PHASE_STRIP = NO;
				DEBUG_INFORMATION_FORMAT = dwarf;
				ENABLE_STRICT_OBJC_MSGSEND = YES;
				ENABLE_TESTABILITY = YES;
				ENABLE_USER_SCRIPT_SANDBOXING = YES;
				GCC_C_LANGUAGE_STANDARD = gnu17;
				GCC_DYNAMIC_NO_PIC = NO;
				GCC_NO_COMMON_BLOCKS = YES;
				GCC_OPTIMIZATION_LEVEL = 0;
				GCC_PREPROCESSOR_DEFINITIONS = (
					"DEBUG=1",
					"$(inherited)",
				);
				GCC_WARN_64_TO_32_BIT_CONVERSION = YES;
				GCC_WARN_ABOUT_RETURN_TYPE = YES_ERROR;
				GCC_WARN_UNDECLARED_SELECTOR = YES;
				GCC_WARN_UNINITIALIZED_AUTOS = YES_AGGRESSIVE;
				GCC_WARN_UNUSED_FUNCTION = YES;
				GCC_WARN_UNUSED_VARIABLE = YES;
				IPHONEOS_DEPLOYMENT_TARGET = 16.0;
				LOCALIZATION_PREFERS_STRING_CATALOGS = YES;
				MTL_ENABLE_DEBUG_INFO = INCLUDE_SOURCE;
				MTL_FAST_MATH = YES;
				ONLY_ACTIVE_ARCH = YES;
				SDKROOT = iphoneos;
				SWIFT_ACTIVE_COMPILATION_CONDITIONS = "DEBUG $(inherited)";
				SWIFT_OPTIMIZATION_LEVEL = "-Onone";
			}};
			name = Debug;
		}};
		{build_config_release_project} /* Release */ = {{
			isa = XCBuildConfiguration;
			buildSettings = {{
				ALWAYS_SEARCH_USER_PATHS = NO;
				ASSETCATALOG_COMPILER_GENERATE_SWIFT_ASSET_SYMBOL_EXTENSIONS = YES;
				CLANG_ANALYZER_NONNULL = YES;
				CLANG_ANALYZER_NUMBER_OBJECT_CONVERSION = YES_AGGRESSIVE;
				CLANG_CXX_LANGUAGE_STANDARD = "gnu++20";
				CLANG_ENABLE_MODULES = YES;
				CLANG_ENABLE_OBJC_ARC = YES;
				CLANG_ENABLE_OBJC_WEAK = YES;
				CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING = YES;
				CLANG_WARN_BOOL_CONVERSION = YES;
				CLANG_WARN_COMMA = YES;
				CLANG_WARN_CONSTANT_CONVERSION = YES;
				CLANG_WARN_DEPRECATED_OBJC_IMPLEMENTATIONS = YES;
				CLANG_WARN_DIRECT_OBJC_ISA_USAGE = YES_ERROR;
				CLANG_WARN_DOCUMENTATION_COMMENTS = YES;
				CLANG_WARN_EMPTY_BODY = YES;
				CLANG_WARN_ENUM_CONVERSION = YES;
				CLANG_WARN_INFINITE_RECURSION = YES;
				CLANG_WARN_INT_CONVERSION = YES;
				CLANG_WARN_NON_LITERAL_NULL_CONVERSION = YES;
				CLANG_WARN_OBJC_IMPLICIT_RETAIN_SELF = YES;
				CLANG_WARN_OBJC_LITERAL_CONVERSION = YES;
				CLANG_WARN_OBJC_ROOT_CLASS = YES_ERROR;
				CLANG_WARN_QUOTED_INCLUDE_IN_FRAMEWORK_HEADER = YES;
				CLANG_WARN_RANGE_LOOP_ANALYSIS = YES;
				CLANG_WARN_STRICT_PROTOTYPES = YES;
				CLANG_WARN_SUSPICIOUS_MOVE = YES;
				CLANG_WARN_UNGUARDED_AVAILABILITY = YES_AGGRESSIVE;
				CLANG_WARN_UNREACHABLE_CODE = YES;
				CLANG_WARN__DUPLICATE_METHOD_MATCH = YES;
				COPY_PHASE_STRIP = NO;
				DEBUG_INFORMATION_FORMAT = "dwarf-with-dsym";
				ENABLE_NS_ASSERTIONS = NO;
				ENABLE_STRICT_OBJC_MSGSEND = YES;
				ENABLE_USER_SCRIPT_SANDBOXING = YES;
				GCC_C_LANGUAGE_STANDARD = gnu17;
				GCC_NO_COMMON_BLOCKS = YES;
				GCC_WARN_64_TO_32_BIT_CONVERSION = YES;
				GCC_WARN_ABOUT_RETURN_TYPE = YES_ERROR;
				GCC_WARN_UNDECLARED_SELECTOR = YES;
				GCC_WARN_UNINITIALIZED_AUTOS = YES_AGGRESSIVE;
				GCC_WARN_UNUSED_FUNCTION = YES;
				GCC_WARN_UNUSED_VARIABLE = YES;
				IPHONEOS_DEPLOYMENT_TARGET = 16.0;
				LOCALIZATION_PREFERS_STRING_CATALOGS = YES;
				MTL_ENABLE_DEBUG_INFO = NO;
				MTL_FAST_MATH = YES;
				SDKROOT = iphoneos;
				SWIFT_COMPILATION_MODE = wholemodule;
				VALIDATE_PRODUCT = YES;
			}};
			name = Release;
		}};
		{build_config_debug_target} /* Debug */ = {{
			isa = XCBuildConfiguration;
			buildSettings = {{
				ASSETCATALOG_COMPILER_APPICON_NAME = AppIcon;
				ASSETCATALOG_COMPILER_GLOBAL_ACCENT_COLOR_NAME = AccentColor;
				CODE_SIGN_ENTITLEMENTS = Resources/termi.entitlements;
				CODE_SIGN_STYLE = Manual;
				CURRENT_PROJECT_VERSION = 1;
				DEVELOPMENT_TEAM = "";
				ENABLE_PREVIEWS = YES;
				GENERATE_INFOPLIST_FILE = NO;
				INFOPLIST_FILE = Resources/Info.plist;
				INFOPLIST_KEY_UIApplicationSceneManifest_Generation = YES;
				INFOPLIST_KEY_UIApplicationSupportsIndirectInputEvents = YES;
				INFOPLIST_KEY_UILaunchScreen_Generation = YES;
				INFOPLIST_KEY_UISupportedInterfaceOrientations = "UIInterfaceOrientationPortrait UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight";
				INFOPLIST_KEY_UISupportedInterfaceOrientations_iPad = "UIInterfaceOrientationPortrait UIInterfaceOrientationPortraitUpsideDown UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight";
				INFOPLIST_KEY_UISupportedInterfaceOrientations_iPhone = "UIInterfaceOrientationPortrait UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight";
				LD_RUNPATH_SEARCH_PATHS = (
					"$(inherited)",
					"@executable_path/Frameworks",
				);
				LIBRARY_SEARCH_PATHS = (
					"$(inherited)",
					"$(PROJECT_DIR)/Kernel/syscall",
					"$(PROJECT_DIR)/Filesystem/fakefs",
				);
				HEADER_SEARCH_PATHS = (
					"$(inherited)",
					"$(PROJECT_DIR)/Kernel/include",
					"$(PROJECT_DIR)/Kernel/syscall",
					"$(PROJECT_DIR)/Filesystem",
				);
				MARKETING_VERSION = 1.0;
				OTHER_LDFLAGS = (
					"-lsqlite3",
					"-lsyscall",
				);
				PRODUCT_BUNDLE_IDENTIFIER = {bundle_id};
				PRODUCT_NAME = "$(TARGET_NAME)";
				PROVISIONING_PROFILE_SPECIFIER = "";
				SUPPORTED_PLATFORMS = "iphoneos iphonesimulator";
				SUPPORTS_MACCATALYST = NO;
				SUPPORTS_MAC_DESIGNED_FOR_IPHONE_IPAD = NO;
				SUPPORTS_XR_DESIGNED_FOR_IPHONE_IPAD = NO;
				SWIFT_EMIT_LOC_STRINGS = YES;
				SWIFT_OBJC_BRIDGING_HEADER = "Bridge/termi-Bridging-Header.h";
				SWIFT_VERSION = 5.0;
				TARGETED_DEVICE_FAMILY = "1,2";
			}};
			name = Debug;
		}};
		{build_config_release_target} /* Release */ = {{
			isa = XCBuildConfiguration;
			buildSettings = {{
				ASSETCATALOG_COMPILER_APPICON_NAME = AppIcon;
				ASSETCATALOG_COMPILER_GLOBAL_ACCENT_COLOR_NAME = AccentColor;
				CODE_SIGN_ENTITLEMENTS = Resources/termi.entitlements;
				CODE_SIGN_STYLE = Manual;
				CURRENT_PROJECT_VERSION = 1;
				DEVELOPMENT_TEAM = "";
				ENABLE_PREVIEWS = YES;
				GENERATE_INFOPLIST_FILE = NO;
				INFOPLIST_FILE = Resources/Info.plist;
				INFOPLIST_KEY_UIApplicationSceneManifest_Generation = YES;
				INFOPLIST_KEY_UIApplicationSupportsIndirectInputEvents = YES;
				INFOPLIST_KEY_UILaunchScreen_Generation = YES;
				INFOPLIST_KEY_UISupportedInterfaceOrientations = "UIInterfaceOrientationPortrait UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight";
				INFOPLIST_KEY_UISupportedInterfaceOrientations_iPad = "UIInterfaceOrientationPortrait UIInterfaceOrientationPortraitUpsideDown UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight";
				INFOPLIST_KEY_UISupportedInterfaceOrientations_iPhone = "UIInterfaceOrientationPortrait UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight";
				LD_RUNPATH_SEARCH_PATHS = (
					"$(inherited)",
					"@executable_path/Frameworks",
				);
				LIBRARY_SEARCH_PATHS = (
					"$(inherited)",
					"$(PROJECT_DIR)/Kernel/syscall",
					"$(PROJECT_DIR)/Filesystem/fakefs",
				);
				HEADER_SEARCH_PATHS = (
					"$(inherited)",
					"$(PROJECT_DIR)/Kernel/include",
					"$(PROJECT_DIR)/Kernel/syscall",
					"$(PROJECT_DIR)/Filesystem",
				);
				MARKETING_VERSION = 1.0;
				OTHER_LDFLAGS = (
					"-lsqlite3",
					"-lsyscall",
				);
				PRODUCT_BUNDLE_IDENTIFIER = {bundle_id};
				PRODUCT_NAME = "$(TARGET_NAME)";
				PROVISIONING_PROFILE_SPECIFIER = "";
				SUPPORTED_PLATFORMS = "iphoneos iphonesimulator";
				SUPPORTS_MACCATALYST = NO;
				SUPPORTS_MAC_DESIGNED_FOR_IPHONE_IPAD = NO;
				SUPPORTS_XR_DESIGNED_FOR_IPHONE_IPAD = NO;
				SWIFT_EMIT_LOC_STRINGS = YES;
				SWIFT_OBJC_BRIDGING_HEADER = "Bridge/termi-Bridging-Header.h";
				SWIFT_VERSION = 5.0;
				TARGETED_DEVICE_FAMILY = "1,2";
			}};
			name = Release;
		}};
/* End XCBuildConfiguration section */

/* Begin XCConfigurationList section */
		{build_config_list_project} /* Build configuration list for PBXProject "termi" */ = {{
			isa = XCConfigurationList;
			buildConfigurations = (
				{build_config_debug_project} /* Debug */,
				{build_config_release_project} /* Release */,
			);
			defaultConfigurationIsVisible = 0;
			defaultConfigurationName = Release;
		}};
		{build_config_list_target} /* Build configuration list for PBXNativeTarget "termi" */ = {{
			isa = XCConfigurationList;
			buildConfigurations = (
				{build_config_debug_target} /* Debug */,
				{build_config_release_target} /* Release */,
			);
			defaultConfigurationIsVisible = 0;
			defaultConfigurationName = Release;
		}};
/* End XCConfigurationList section */
	}};
	rootObject = {project_obj_id} /* Project object */;
}}
"""

xcodeproj_dir = project_root / f"{project_name}.xcodeproj"
xcodeproj_dir.mkdir(exist_ok=True)

with open(xcodeproj_dir / "project.pbxproj", "w") as f:
    f.write(pbxproj_content)

print(f"Created {xcodeproj_dir}/project.pbxproj")
