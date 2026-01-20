import Foundation

class EmulatorBridge {
    static let shared = EmulatorBridge()

    private var sessionMap: [UUID: TerminalSession] = [:]
    private var emulatorQueue = DispatchQueue(label: "com.termi.emulator", qos: .userInteractive)
    private var isInitialized = false

    private init() {
        print("🏗️  [EmulatorBridge] INIT - EmulatorBridge singleton created")
    }

    func initialize() {
        print("🚀 [EmulatorBridge] initialize - Starting initialization")
        print("📊 [EmulatorBridge] initialize - isInitialized: \(isInitialized)")
        guard !isInitialized else {
            print("⚠️  [EmulatorBridge] initialize - Already initialized, skipping")
            return
        }
        print("🔄 [EmulatorBridge] initialize - Dispatching to emulator queue")
        emulatorQueue.async {
            print("⚙️  [EmulatorBridge] emulatorQueue - Setting up emulator")
            self.setupEmulator()
            self.isInitialized = true
            print("✅ [EmulatorBridge] emulatorQueue - Initialization complete")
        }
    }

    private func setupEmulator() {
        print("🔧 [EmulatorBridge] setupEmulator - Starting emulator setup")
        initializeFilesystem()
        initializeSyscallLayer()
        startEmulatorLoop()
        print("✅ [EmulatorBridge] setupEmulator - Emulator setup complete")
    }

    private func initializeFilesystem() {
        print("💾 [EmulatorBridge] initializeFilesystem - Initializing filesystem")
        
        guard let bundlePath = Bundle.main.resourcePath else {
            print("❌ [EmulatorBridge] initializeFilesystem - Failed to get bundle path")
            return
        }
        
        let alpineRootfs = (bundlePath as NSString).appendingPathComponent("Alpine/rootfs")
        let metaDB = (alpineRootfs as NSString).appendingPathComponent("meta.db")
        let dataPath = (alpineRootfs as NSString).appendingPathComponent("data")
        
        print("📂 [EmulatorBridge] initializeFilesystem - Alpine rootfs: \(alpineRootfs)")
        print("📂 [EmulatorBridge] initializeFilesystem - Meta DB: \(metaDB)")
        print("📂 [EmulatorBridge] initializeFilesystem - Data path: \(dataPath)")
        
        let dbExists = FileManager.default.fileExists(atPath: metaDB)
        print("📊 [EmulatorBridge] initializeFilesystem - DB exists: \(dbExists)")
        
        if !dbExists {
            print("❌ [EmulatorBridge] initializeFilesystem - Alpine rootfs not found! Need to run setup_alpine.sh")
            return
        }
        
        print("✅ [EmulatorBridge] initializeFilesystem - Filesystem initialization complete")
    }



    private func initializeSyscallLayer() {
        print("🔧 [EmulatorBridge] initializeSyscallLayer - Syscall layer initialization (stub)")
    }

    private func startEmulatorLoop() {
        print("🔄 [EmulatorBridge] startEmulatorLoop - Emulator loop starting (stub)")
    }

    func attachSession(_ session: TerminalSession) {
        print("🔗 [EmulatorBridge] attachSession - Attaching session: \(session.id)")
        sessionMap[session.id] = session
        print("📊 [EmulatorBridge] attachSession - Total sessions in map: \(sessionMap.count)")

        emulatorQueue.async {
            print("🔄 [EmulatorBridge] emulatorQueue - Creating PTY for session: \(session.id)")
            self.createPTY(for: session)
        }
    }

    func detachSession(_ session: TerminalSession) {
        print("🔌 [EmulatorBridge] detachSession - Detaching session: \(session.id)")
        emulatorQueue.async {
            print("🔄 [EmulatorBridge] emulatorQueue - Destroying PTY for session: \(session.id)")
            self.destroyPTY(for: session)
            self.sessionMap.removeValue(forKey: session.id)
            print("📊 [EmulatorBridge] emulatorQueue - Remaining sessions: \(self.sessionMap.count)")
        }
    }

    private func createPTY(for session: TerminalSession) {
        print("🖥️  [EmulatorBridge] createPTY - Creating PTY for session: \(session.id)")
        let welcomeMessage = """
        Welcome to termi
        ARM64 Linux Terminal for iOS 26

        [EMULATOR NOT WIRED UP YET]
        Need to initialize:
        - fakefs with Alpine rootfs
        - ARM64 emulator
        - Load /bin/sh binary
        - Wire up PTY I/O
        
        \(session.workingDirectory) $ 
        """

        print("📝 [EmulatorBridge] createPTY - Welcome message length: \(welcomeMessage.count)")
        if let data = welcomeMessage.data(using: .utf8) {
            print("📤 [EmulatorBridge] createPTY - Sending welcome message (\(data.count) bytes)")
            DispatchQueue.main.async {
                session.receiveOutput(data)
            }
        } else {
            print("❌ [EmulatorBridge] createPTY - Failed to encode welcome message")
        }
    }

    private func destroyPTY(for session: TerminalSession) {
        print("🗑️  [EmulatorBridge] destroyPTY - Destroying PTY for session: \(session.id)")
    }

    func sendInput(_ input: String, to session: TerminalSession) {
        print("⌨️  [EmulatorBridge] sendInput - Session: \(session.id.uuidString.prefix(8)), Input length: \(input.count)")
        print("📝 [EmulatorBridge] sendInput - Input: \(input.prefix(100))")
        emulatorQueue.async {
            print("🔄 [EmulatorBridge] emulatorQueue - Processing input for session: \(session.id.uuidString.prefix(8))")
            self.processInput(input, for: session)
        }
    }

    private func processInput(_ input: String, for session: TerminalSession) {
        print("🔄 [EmulatorBridge] processInput - Processing for session: \(session.id.uuidString.prefix(8))")
        print("📝 [EmulatorBridge] processInput - TODO: Wire up ARM64 emulator to execute: '\(input.trimmingCharacters(in: .whitespacesAndNewlines))'")
        
        if let data = input.data(using: .utf8) {
            print("📤 [EmulatorBridge] processInput - Echoing input (\(data.count) bytes)")
            DispatchQueue.main.async {
                session.receiveOutput(data)
            }
        } else {
            print("❌ [EmulatorBridge] processInput - Failed to encode input as UTF-8")
        }

        if input.contains("\n") {
            print("↩️  [EmulatorBridge] processInput - Newline detected, sending prompt")
            let prompt = "\n\(session.workingDirectory) $ "
            if let data = prompt.data(using: .utf8) {
                print("📤 [EmulatorBridge] processInput - Sending prompt (\(data.count) bytes)")
                DispatchQueue.main.async {
                    session.receiveOutput(data)
                }
            } else {
                print("❌ [EmulatorBridge] processInput - Failed to encode prompt")
            }
        }
    }

    func resize(session: TerminalSession, cols: Int, rows: Int) {
        print("📐 [EmulatorBridge] resize - Session: \(session.id.uuidString.prefix(8)), Cols: \(cols), Rows: \(rows)")
        emulatorQueue.async {
            print("🔄 [EmulatorBridge] emulatorQueue - Resize operation (stub)")
        }
    }


}
