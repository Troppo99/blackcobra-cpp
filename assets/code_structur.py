import os

# === KONFIGURASI ===
root_dir = r"C:\coding\blackcobra-cpp"
output_file = "assets/directory_structure.txt"

# Folder dan file yang ingin diabaikan
excluded_folders = {".git", ".venv", "archives", "assets"}
excluded_files = {}


# === Fungsi Rekursif untuk Menyusun Struktur ===
def write_structure(out, current_path, prefix=""):
    entries = sorted(os.listdir(current_path))

    entries = [e for e in entries if e not in excluded_files and e not in excluded_folders]

    for i, entry in enumerate(entries):
        path = os.path.join(current_path, entry)
        connector = "└── " if i == len(entries) - 1 else "├── "
        out.write(f"{prefix}{connector}{entry}\n")

        if os.path.isdir(path) and entry not in excluded_folders:
            extension = "    " if i == len(entries) - 1 else "│   "
            write_structure(out, path, prefix + extension)


# === Eksekusi ===
with open(output_file, "w", encoding="utf-8") as f:
    f.write(f"{os.path.basename(root_dir)}\n")
    write_structure(f, root_dir)

print(f"✅ Struktur direktori berhasil disimpan di: {output_file}")
