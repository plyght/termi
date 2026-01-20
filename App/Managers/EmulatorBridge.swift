import Foundation

class EmulatorBridge {
    static let shared = EmulatorBridge()

    private var sessionMap: [UUID: TerminalSession] = [:]
    private var sessionEmulators: [UUID: UnsafeMutableRawPointer] = [:]
    private var emulatorQueue = DispatchQueue(label: "com.termi.emulator", qos: .userInteractive)
    private var isInitialized = false

    private init() {
        print("🏗️  [EmulatorBridge] INIT - EmulatorBridge singleton created")
    }
    
    deinit {
        print("🗑️  [EmulatorBridge] DEINIT - Cleaning up filesystem")
        emulator_deinit_filesystem()
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
        
        let alpineRootfs = (bundlePath as NSString).appendingPathComponent("rootfs")
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
        
        let result = metaDB.withCString { dbPathPtr in
            dataPath.withCString { dataPathPtr in
                emulator_init_filesystem(dbPathPtr, dataPathPtr)
            }
        }
        
        if result == 0 {
            print("✅ [EmulatorBridge] initializeFilesystem - Filesystem initialized successfully")
        } else {
            print("❌ [EmulatorBridge] initializeFilesystem - Filesystem initialization failed with code: \(result)")
        }
    }



    private func initializeSyscallLayer() {
        print("🔧 [EmulatorBridge] initializeSyscallLayer - Syscalls already wired in C layer")
    }

    private func startEmulatorLoop() {
        print("🔄 [EmulatorBridge] startEmulatorLoop - Each session starts its own loop")
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
        
        guard let handle = emulator_create() else {
            print("❌ [EmulatorBridge] createPTY - Failed to create emulator handle")
            return
        }
        
        print("✅ [EmulatorBridge] createPTY - Emulator handle created: \(handle)")
        sessionEmulators[session.id] = handle
        
        let shellPath = "/bin/busybox"
        let loadResult = shellPath.withCString { pathPtr in
            emulator_load_shell(handle, pathPtr)
        }
        
        if loadResult != 0 {
            print("❌ [EmulatorBridge] createPTY - Failed to load shell: \(shellPath), error: \(loadResult)")
            emulator_destroy(handle)
            sessionEmulators.removeValue(forKey: session.id)
            return
        }
        
        print("✅ [EmulatorBridge] createPTY - Shell loaded: \(shellPath)")
        
        let runResult = emulator_run_async(handle)
        if runResult != 0 {
            print("❌ [EmulatorBridge] createPTY - Failed to start emulator async, error: \(runResult)")
            emulator_destroy(handle)
            sessionEmulators.removeValue(forKey: session.id)
            return
        }
        
        print("✅ [EmulatorBridge] createPTY - Emulator running async")
        
        startOutputPolling(for: session, handle: handle)
    }
    
    private func startOutputPolling(for session: TerminalSession, handle: UnsafeMutableRawPointer) {
        print("📡 [EmulatorBridge] startOutputPolling - Starting output polling for session: \(session.id)")
        
        emulatorQueue.asyncAfter(deadline: .now() + 0.1) { [weak self] in
            guard let self = self else { return }
            guard self.sessionEmulators[session.id] != nil else {
                print("🛑 [EmulatorBridge] startOutputPolling - Session \(session.id) no longer active")
                return
            }
            
            var buffer = [CChar](repeating: 0, count: 4096)
            let bytesRead = emulator_read_output(handle, &buffer, 4096)
            
            if bytesRead > 0 {
                print("📥 [EmulatorBridge] startOutputPolling - Read \(bytesRead) bytes from emulator")
                let data = Data(bytes: buffer, count: Int(bytesRead))
                DispatchQueue.main.async {
                    session.receiveOutput(data)
                }
            }
            
            self.startOutputPolling(for: session, handle: handle)
        }
    }

    private func destroyPTY(for session: TerminalSession) {
        print("🗑️  [EmulatorBridge] destroyPTY - Destroying PTY for session: \(session.id)")
        
        guard let handle = sessionEmulators[session.id] else {
            print("⚠️  [EmulatorBridge] destroyPTY - No emulator handle found for session: \(session.id)")
            return
        }
        
        print("🛑 [EmulatorBridge] destroyPTY - Stopping emulator")
        emulator_stop(handle)
        
        print("🗑️  [EmulatorBridge] destroyPTY - Destroying emulator handle")
        emulator_destroy(handle)
        
        sessionEmulators.removeValue(forKey: session.id)
        print("✅ [EmulatorBridge] destroyPTY - PTY destroyed successfully")
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
        print("📝 [EmulatorBridge] processInput - Input: '\(input)'")
        
        guard let handle = sessionEmulators[session.id] else {
            print("❌ [EmulatorBridge] processInput - No emulator handle found for session: \(session.id)")
            return
        }
        
        guard let data = input.data(using: .utf8) else {
            print("❌ [EmulatorBridge] processInput - Failed to encode input as UTF-8")
            return
        }
        
        let result = data.withUnsafeBytes { bufferPtr in
            guard let baseAddress = bufferPtr.baseAddress else { return Int32(-1) }
            return emulator_send_input(handle, baseAddress.assumingMemoryBound(to: CChar.self), data.count)
        }
        
        if result == 0 {
            print("✅ [EmulatorBridge] processInput - Sent \(data.count) bytes to emulator")
        } else {
            print("❌ [EmulatorBridge] processInput - Failed to send input, error: \(result)")
        }
    }

    func resize(session: TerminalSession, cols: Int, rows: Int) {
        print("📐 [EmulatorBridge] resize - Session: \(session.id.uuidString.prefix(8)), Cols: \(cols), Rows: \(rows)")
        emulatorQueue.async {
            print("🔄 [EmulatorBridge] emulatorQueue - Resize operation (stub)")
        }
    }


}
