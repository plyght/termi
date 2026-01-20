import SwiftUI

struct ThemePickerView: View {
    @EnvironmentObject var themeManager: ThemeManager
    @Environment(\.dismiss) var dismiss
    
    var body: some View {
        NavigationView {
            List(TerminalTheme.allThemes, id: \.name) { theme in
                ThemePreviewRow(theme: theme, isSelected: themeManager.currentTheme.name == theme.name)
                    .contentShape(Rectangle())
                    .onTapGesture {
                        themeManager.setTheme(theme)
                    }
            }
            .navigationTitle("Choose Theme")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Done") {
                        dismiss()
                    }
                }
            }
        }
    }
}

struct ThemePreviewRow: View {
    let theme: TerminalTheme
    let isSelected: Bool
    
    var body: some View {
        HStack(spacing: 12) {
            VStack(alignment: .leading, spacing: 4) {
                Text(theme.name)
                    .font(.system(size: 16, weight: .medium))
                
                HStack(spacing: 4) {
                    ForEach(0..<8, id: \.self) { index in
                        Circle()
                            .fill(theme.ansiColors[index])
                            .frame(width: 16, height: 16)
                    }
                }
            }
            
            Spacer()
            
            if isSelected {
                Image(systemName: "checkmark.circle.fill")
                    .foregroundColor(.blue)
                    .font(.system(size: 22))
            }
        }
        .padding(.vertical, 8)
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(theme.backgroundColor)
                .padding(.horizontal, -16)
                .padding(.vertical, -8)
        )
    }
}
