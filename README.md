# Giới thiệu
* Một game caro được viết theo hướng thủ tục thuần bằng C/C++.

# Thông tin đồ án.
* Môn học:   
* Lớp:  
* Giảng viên hướng dẫn: Thầy 3T  
* Nhóm thực hiện:  
* Danh sách thành viên:  


# Clone code & Build
Mấy anh iu clone về nhớ để ý nha

## Clone
Vì có sử dụng thư viện ngoài (Raylib) quản lý bằng git submodule  
Khi clone nhớ thêm cờ `--recursive`

```bash
git clone --recursive <thay_link_github_cua_ban_vao_day>
```

Nếu lỡ `git clone` hay `git pull` thường thì chạy thêm
```bash
git submodule update --init --recursive
```

## Build & Chạy
* Tất cả thực thi tại thư mục gốc

### Tạo cấu hình CMake
* Chỉ chạy lệnh này vào lần đầu tiên hoặc sau khi thêm file `.cpp` mới vào `./src/`
```bash
cmake -S . -B build
```

### Build
```bash
cmake --build build
```

### Run
* Trên Unix:
```bash
./build/GameCaro
```
* Trên Windows hình như nó nhét file `.exe` vào `Debug/` thì phải
```bash
./build/Debug/GameCaro.exe
```
