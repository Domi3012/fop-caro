# 0. Giới thiệu
* Một game caro được viết theo hướng thủ tục thuần bằng C/C++, sử dụng thư viện Raylib.

# 1. Thông tin đồ án.
* Môn học:   
* Lớp:  
* Giảng viên hướng dẫn: Thầy 3T  
* Nhóm thực hiện:  
* Danh sách thành viên:  


# 2. Clone code & Build
* Mấy anh iu clone về nhớ để ý nha

## 2.1. Clone
* Vì có sử dụng thư viện ngoài (Raylib) quản lý bằng git submodule nên khi clone nhớ thêm cờ `--recursive`

```bash
git clone --recursive <link-github-project>
```

* Nếu lỡ `git clone` hay `git pull` thường thì chạy thêm
```bash
git submodule update --init --recursive
```

## 2.2. Build
* Tất cả thực thi tại thư mục gốc

### 2.2.1. Tạo cấu hình CMake
* Chỉ chạy lệnh này vào lần đầu tiên hoặc sau khi thêm file `.cpp` mới vào `./src/`
```bash
cmake -S . -B build
```

### 2.2.2. Build
```bash
cmake --build build
```

## 2.3. Run
* Trên Unix:
```bash
./build/GameCaro
```
* Trên Windows hình như nó nhét file `.exe` vào `Debug/` thì phải
```bash
./build/Debug/GameCaro.exe
```
