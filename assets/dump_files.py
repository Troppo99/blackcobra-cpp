import os

# === KONFIGURASI ===
root_dir = r"C:\coding\blackcobra-cpp"
output_file = "assets/my_codes.txt"

# Folder atau file yang ingin dikecualikan (bisa ditambah sesuai kebutuhan)
excluded_folders = [".git", ".venv", "archives", "assets"]
excluded_files = []

# === PROSES ===
file_counter = 1

with open(output_file, "w", encoding="utf-8") as out:
    for dirpath, dirnames, filenames in os.walk(root_dir):
        # Filter folder yang dikecualikan
        dirnames[:] = [d for d in dirnames if d not in excluded_folders]

        for filename in filenames:
            if filename in excluded_files:
                continue

            file_path = os.path.join(dirpath, filename)
            try:
                with open(file_path, "r", encoding="utf-8") as f:
                    content = f.read()

                out.write(f"file {file_counter}: {file_path}\n")
                out.write(content)
                out.write("\n\n")
                file_counter += 1

            except Exception as e:
                print(f"[!] Gagal membaca file: {file_path} ({e})")
                continue

print(f"✅ Selesai. Total file: {file_counter - 1}. Disimpan ke {output_file}")
