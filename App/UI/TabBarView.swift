import SwiftUI

struct TabBarView: View {
    @Binding var selectedTab: Int
    @EnvironmentObject var sessionManager: SessionManager
    @EnvironmentObject var themeManager: ThemeManager
    @State private var showNewTabSheet = false
    
    var body: some View {
        ScrollView(.horizontal, showsIndicators: false) {
            HStack(spacing: 8) {
                ForEach(Array(sessionManager.sessions.enumerated()), id: \.offset) { index, session in
                    TabButton(
                        title: "Terminal \(index + 1)",
                        isSelected: selectedTab == index,
                        onTap: {
                            selectedTab = index
                        },
                        onClose: {
                            sessionManager.closeSession(at: index)
                            if selectedTab >= sessionManager.sessions.count {
                                selectedTab = max(0, sessionManager.sessions.count - 1)
                            }
                        }
                    )
                }
                
                Button(action: {
                    sessionManager.createSession()
                    selectedTab = sessionManager.sessions.count - 1
                }) {
                    Image(systemName: "plus.circle.fill")
                        .font(.system(size: 24))
                        .foregroundColor(themeManager.currentTheme.foregroundColor.opacity(0.6))
                }
                .padding(.leading, 4)
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
        }
        .background(themeManager.currentTheme.backgroundColor.opacity(0.95))
        .frame(height: 50)
    }
}

struct TabButton: View {
    let title: String
    let isSelected: Bool
    let onTap: () -> Void
    let onClose: () -> Void
    @EnvironmentObject var themeManager: ThemeManager
    
    var body: some View {
        HStack(spacing: 8) {
            Text(title)
                .font(.system(size: 14, weight: isSelected ? .semibold : .regular, design: .monospaced))
                .foregroundColor(
                    isSelected 
                    ? themeManager.currentTheme.foregroundColor 
                    : themeManager.currentTheme.foregroundColor.opacity(0.6)
                )
            
            Button(action: onClose) {
                Image(systemName: "xmark.circle.fill")
                    .font(.system(size: 16))
                    .foregroundColor(themeManager.currentTheme.foregroundColor.opacity(0.4))
            }
            .buttonStyle(.plain)
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 6)
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(
                    isSelected 
                    ? themeManager.currentTheme.foregroundColor.opacity(0.1)
                    : Color.clear
                )
        )
        .overlay(
            RoundedRectangle(cornerRadius: 8)
                .stroke(
                    isSelected 
                    ? themeManager.currentTheme.foregroundColor.opacity(0.3)
                    : Color.clear,
                    lineWidth: 1
                )
        )
        .onTapGesture(perform: onTap)
    }
}
