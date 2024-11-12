#include <iostream>
#include <random>
#include <vector>
#include <algorithm>
#include <ctime>
#include <stdexcept>
#include <string>
#include <chrono>
#include <cstdint>
#include <thread>

// Lớp BigInt512 để lưu trữ và thao tác với số lớn 512 bit
class BigInt512 {
private:
    uint64_t parts[8] = {}; // 512-bit chia thành 8 phần 64-bit


public:
    uint64_t getpart(int i){
        return parts[i];
    }

    //constructor
    BigInt512() = default;

    BigInt512(uint64_t num) {
        parts[0] = num;
    }

    BigInt512(const std::string& hexStr) {
        int hexLength = hexStr.size();
        int partIndex = 0;
        for (int i = hexLength; i > 0; i -= 16) {
            std::string partStr = hexStr.substr(std::max(0, i - 16), std::min(16, i));
            parts[partIndex++] = std::stoull(partStr, nullptr, 16);
        }
    }

    //Lấy độ dài chuỗi
    int get_bit_length() const {
    // Tìm phần không phải 0 cao nhất
    for (int i = 7; i >= 0; i--) {
        if (parts[i] != 0) {
            // Tìm bit cao nhất trong phần này
            int bit_pos = 63;
            uint64_t part = parts[i];
            while (bit_pos >= 0 && !(part & (1ULL << bit_pos))) {
                bit_pos--;
            }
            // Độ dài bit = vị trí phần * 64 + vị trí bit + 1
            return i * 64 + bit_pos + 1;
        }
    }
    return 0; // Trường hợp số = 0
}

    //Hàm tạo số ngẫu nhiên
    static BigInt512 generate_random_number(int bits) {
    BigInt512 result;
    std::random_device rd;
    auto time_seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::seed_seq seeds{
        rd(), rd(), rd(), rd(),rd(),rd(),rd(),
        static_cast<unsigned int>(time_seed),
        static_cast<unsigned int>(time_seed >> 32),
        static_cast<unsigned int>(std::hash<std::thread::id>{}(std::this_thread::get_id())),
        static_cast<unsigned int>(reinterpret_cast<uintptr_t>(&seeds)) // Stack address entropy
    };
    std::mt19937_64 gen1(seeds);
    // Sử dụng uniform_int_distribution để tạo số từ 0 đến 5000
    std::uniform_int_distribution<uint64_t> dist(0, 5000);

    // Chỉ cần điền phần đầu tiên của parts
    result.parts[0] = dist(gen1);

    // Đặt các phần còn lại về 0
    for (int i = 1; i < 8; i++) {
        result.parts[i] = 0;
    }
    return result;
    }

    //Toán tử <<
    friend std::ostream& operator<<(std::ostream& os, const BigInt512& num) {
        for (int i = 7; i >= 0; --i) {
            os << std::dec << num.parts[i];
        }
        return os;
    }

    //Toán tử +
    BigInt512 operator+(const BigInt512& other) const {
        BigInt512 result;
        uint64_t carry = 0;
        for (int i = 0; i < 8; ++i) {
            uint64_t sum = parts[i] + other.parts[i] + carry;
            result.parts[i] = sum;
            carry = (sum < parts[i]) || (carry && sum == parts[i]);
        }
        return result;
    }

    //Toán tử -
    BigInt512 operator-(const BigInt512& other) const {
        BigInt512 result;
        uint64_t borrow = 0;
        for (int i = 0; i < 8; ++i) {
            uint64_t diff = parts[i] - other.parts[i] - borrow;
            result.parts[i] = diff;
            borrow = (parts[i] < other.parts[i]) || (borrow && parts[i] == other.parts[i]);
        }
        return result;
    }

    //Toán tử *
    BigInt512 operator*(const BigInt512& other) const {
        BigInt512 result;
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; i + j < 8; ++j) {
                unsigned __int128 product = static_cast<unsigned __int128>(parts[i]) * other.parts[j];
                result.parts[i + j] += static_cast<uint64_t>(product);
                if (i + j + 1 < 8)
                    result.parts[i + j + 1] += static_cast<uint64_t>(product >> 64);
            }
        }
        return result;
    }


    //Toán tử /
    BigInt512 operator/(const BigInt512& other) const {
    if (other == BigInt512(0)) {
        throw std::runtime_error("Division by zero");
    }

    BigInt512 quotient(0);
    BigInt512 dividend = *this;
    BigInt512 divisor = other;

    // Trường hợp đặc biệt
    if (dividend <= divisor) {
            if (dividend == divisor)
                return BigInt512(1);
            else
                return BigInt512(0);
    }

    // Tìm vị trí bit cao nhất của divisor
    int leadingBits = 0;
    BigInt512 temp = divisor;
    while (temp != BigInt512(0)) {
        temp = temp >> 1;
        leadingBits++;
    }

    // Tìm vị trí bit cao nhất của dividend
    int dividendBits = 0;
    temp = dividend;
    while (temp != BigInt512(0)) {
        temp = temp >> 1;
        dividendBits++;
    }

    // Số lượng bit cần dịch
    int shift = dividendBits - leadingBits;

    // Dịch divisor sang trái
    divisor = divisor << shift;

    // Thực hiện phép chia bằng cách trừ lặp đi lặp lại
    for (int i = 0; i <= shift; i++) {
        if (dividend >= divisor) {
            dividend = dividend - divisor;
            quotient = quotient + (BigInt512(1) << (shift - i));
        }
        divisor = divisor >> 1;
    }

    return quotient;
    }

    //Toán tử &
    BigInt512 operator&(const BigInt512& other) const {
    BigInt512 result;
    // Thực hiện phép AND bit cho từng phần 64-bit
    for (int i = 0; i < 8; ++i) {
        result.parts[i] = parts[i] & other.parts[i];
    }
    return result;
    }

    // Định nghĩa thêm toán tử &= để sử dụng thuận tiện
    BigInt512& operator&=(const BigInt512& other) {
    for (int i = 0; i < 8; ++i) {
        parts[i] &= other.parts[i];
    }
    return *this;
    }

    //Toán tử %
    BigInt512 operator% (const BigInt512& other) const{
    BigInt512 result = *this;
    result = result - other*(result/other);
    return result;
    }

    //Toán tử ==
    bool operator==(const BigInt512& other) const {
        for (int i = 0; i < 8; ++i) {
            if (parts[i] != other.parts[i]) return false;
        }
        return true;
    }

    //Toán tử !=
    bool operator !=(const BigInt512& other) const {
        for (int i = 0; i < 8; ++i) {
            if (parts[i] != other.parts[i]) return true;
        }
        return false;
    }

    //Toán tử >
    bool operator >(const BigInt512& other) const {
    for (int i = 7; i >= 0; --i) {
        if (parts[i] != other.parts[i]) return parts[i] > other.parts[i];
    }
    return false; // Nếu tất cả phần tử đều bằng nhau, thì không lớn hơn
    }

    //Toán tử >=
    bool operator >=(const BigInt512& other) const {
        for (int i = 7; i >= 0; --i) {
            if (parts[i] != other.parts[i]) return parts[i] > other.parts[i];
        }
        return true;
    }

    //Toán tử <=
    bool operator<=(const BigInt512& other) const {
        for (int i = 7; i >= 0; --i) {
            if (parts[i] != other.parts[i]) return parts[i] < other.parts[i];
        }
        return true;
    }


    //Toán tử right shift
    BigInt512 operator >>(int n) const {
    if (n <= 0) return *this;
    if (n >= 512) return BigInt512(0); // Trả về 0 nếu dịch quá 512 bit

    BigInt512 result;
    int partShift = n / 64;  // Số phần cần dịch hoàn toàn
    int bitShift = n % 64;   // Số bit còn lại cần dịch

    if (bitShift == 0) {
        // Dịch theo từng phần 64-bit
        for (int i = 0; i < 8 - partShift; i++) {
            result.parts[i] = parts[i + partShift];
        }
    } else {
        // Xử lý các phần không phải cuối
        for (int i = 0; i < 7 - partShift; i++) {
            result.parts[i] = (parts[i + partShift] >> bitShift) |
                             (parts[i + partShift + 1] << (64 - bitShift));
        }
        // Xử lý phần cuối
        if (7 - partShift >= 0) {
            result.parts[7 - partShift] = parts[7] >> bitShift;
        }
    }

    // Đảm bảo các phần cao nhất được set về 0
    for (int i = 8 - partShift; i < 8; i++) {
        result.parts[i] = 0;
    }

    //Hàm = 0
    bool isZero = true;
    for (int i = 0; i < 8; i++) {
        if (result.parts[i] != 0) {
            isZero = false;
            break;
        }
    }
    if (isZero) return BigInt512(0);

    return result;
    }

    //Toán tử left shift
    BigInt512 operator <<(int n) const {
    if (n <= 0) return *this;
    if (n >= 512) return BigInt512(0); // Trả về 0 nếu dịch quá 512 bit

    BigInt512 result;
    int partShift = n / 64;  // Số phần cần dịch hoàn toàn
    int bitShift = n % 64;   // Số bit còn lại cần dịch

    if (bitShift == 0) {
        // Dịch theo từng phần 64-bit
        for (int i = 7; i >= partShift; i--) {
            result.parts[i] = parts[i - partShift];
        }
    } else {
        // Xử lý các phần hoàn chỉnh
        for (int i = 7; i > partShift; i--) {
            result.parts[i] = (parts[i - partShift] << bitShift) |
                             (parts[i - partShift - 1] >> (64 - bitShift));
        }
        // Xử lý phần cuối cùng
        if (partShift < 8) {
            result.parts[partShift] = parts[0] << bitShift;
        }
    }

    return result;
    }
};


// A : Trien khai ham tinh luy thua module
// Calculate: (base^exponent) % mod
BigInt512 modular_exponentiation(BigInt512 base, BigInt512 exponent, BigInt512 modulus) {
    if (modulus == BigInt512(1)) return BigInt512(0);

    BigInt512 result(1);
    base = base % modulus;

    while (exponent > BigInt512(0)) {
        // Nếu exponent lẻ, nhân result với base
        if (exponent % BigInt512(2) == BigInt512(1))
            result = (result * base) % modulus;

        // Bình phương base và chia đôi exponent
        base = (base * base) % modulus;
        exponent = exponent / BigInt512(2);
    }

    return result;
}



// B : Trien khai ham sinh so nguyen to ngau nhien
    // Prime - References (Max)
    // OEIS: A005385 (50th - 002963) https://oeis.org/A005385
    //
    // OEIS: A059327 (34th - 079979) https://oeis.org/A059327
    // OEIS: A059394 (34th - 296159) https://oeis.org/A059394
    // OEIS: A059395 (32th - 536087) https://oeis.org/A059395
    // OEIS: A059452 (46th - 004283) https://oeis.org/A059452
    // OEIS: A075705 (45th - 014207) https://oeis.org/A075705
    // OEIS: A075706 (40th - 017027) https://oeis.org/A075706
    // OEIS: A075707 (79th - 047963) https://oeis.org/A075707
    // OEIS: A227853 (51th - 000347) https://oeis.org/A227853
    //
    // Prime - References (Limit)
    // OEIS: A014233 (01st - 2047) https://oeis.org/A014233
    //if (n <= 347 && n >= 2963)  return false;    // outside the range of popularity (OEIS: A014233)

// Miller-Rabin Test
bool millerTest(BigInt512 d, BigInt512 n) {
    // Sử dụng các cơ sở nhỏ thay vì số ngẫu nhiên
    // Các số này là 30 số nguyên tố đầu tiên
    const int PRIME_BASES[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41,
                              43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97};

    // Chọn một cơ sở cố định thay vì ngẫu nhiên
    BigInt512 a = BigInt512(PRIME_BASES[rand() % 25]);
    if (a >= n - BigInt512(4)) return true;

    BigInt512 x = modular_exponentiation(a, d, n);

    if (x == BigInt512(1) || x == n - BigInt512(1))
        return true;

    while (d != n - BigInt512(1)) {
        x = (x * x) % n;
        d = d * BigInt512(2);

        if (x == BigInt512(1))
            return false;
        if (x == n - BigInt512(1))
            return true;
    }
    return false;
}

//Hàm xác định số nguyên tố
bool isPrime(BigInt512 n, int k = 5) {
    if (n <= BigInt512(1) || n == BigInt512(4))
        return false;
    if (n <= BigInt512(3))
        return true;

    // Kiểm tra chia hết cho các số nguyên tố nhỏ trước
    const int small_primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47};
    for (int prime : small_primes) {
        if (n == BigInt512(prime))
            return true;
        if (n % BigInt512(prime) == BigInt512(0))
            return false;
    }

    BigInt512 d = n - BigInt512(1);
    while (d % BigInt512(2) == BigInt512(0))
        d = d / BigInt512(2);

    for (int i = 0; i < k; i++) {
        if (!millerTest(d, n))
            return false;
    }
    return true;
}

//Hàm tạo số nguyên tố an toàn
BigInt512 generate_safe_prime(int bit_size) {
    const std::vector<uint64_t> small_primes = {
        2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
        73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151,
        157, 163, 167, 173, 179, 181, 191, 193, 197, 199
    };
    std::cout << "Generating safe prime..." << std::endl;
    int max_attempts = 5000000;
    int attempts = 0;


    while (attempts < max_attempts) {
        attempts++;
        // Tạo q với bit_size-1 bits
        BigInt512 p = BigInt512(0);
        BigInt512 q = BigInt512::generate_random_number(bit_size-1);

        // Đảm bảo q là số lẻ
        if (q % BigInt512(2) == BigInt512(0))
            q = q + BigInt512(1);

        // Kiểm tra chia hết cho các số nguyên tố nhỏ
        bool divisible_by_small_prime = false;
        for (uint64_t prime : small_primes) {
            if (q % BigInt512(prime) == BigInt512(0)) {
                divisible_by_small_prime = true;
                break;
            }
            else if(q==BigInt512(prime)){
                p = q * BigInt512(2) + BigInt512(1);
            }
        }
        if (divisible_by_small_prime) continue;

        // Kiểm tra q có phải số nguyên tố
        if (!isPrime(q))
            continue;

        // Tính p = 2q + 1
        p = q * BigInt512(2) + BigInt512(1);

        // Đảm bảo p đủ số bits
        //if (p.get_bit_length() != bit_size)
            //continue;

        // Kiểm tra p có phải số nguyên tố (giới hạn p < 5000)
        if (isPrime(p, 3) && (p <=5000)) {
            std::cout << "Found safe prime after " << attempts << " attempts" << std::endl;
            return p;
        }
    }
    throw std::runtime_error("Could not generate safe prime after maximum attempts");
}



// C: Trien khai ham sinh khoa rieng ngau nhien
BigInt512 generate_private_key(const BigInt512& p) {

    BigInt512 private_key;
    private_key = (BigInt512::generate_random_number(512) % (p - BigInt512(4))) + BigInt512(2);
    return private_key;
}


// D : Hoan thanh logic trao doi khoa Diffie - Helman
int main() {

    // 1. Sinh so nguyen to lon p va phan tu sinh g
    int bit_size = 512;    // Kich thuoc bit vi du, co the dieu chinh
    BigInt512 p = generate_safe_prime(bit_size);    // Sinh mot so nguyen to
    std::cout<<"p: "<<p<<std::endl;
    BigInt512 g(2);   // Phan tu sinh, sinh vien can tim hieu va chon gia tri khac

    // 2. Sinh khoa rieng cua Alice va Bob
    BigInt512 a = generate_private_key(p);    // Khoa rieng cua Alice
    BigInt512 b = generate_private_key(p);    // Khoa rieng cua Bob
    std::cout<<a<<" "<<b<<std::endl;

    // 3. Tinh gia tri cong khai cua Alice va Bob
    BigInt512 A = modular_exponentiation(g, a, p);    // Alice tinh A = g^a % p
    BigInt512 B = modular_exponentiation(g, b, p);    // Bob tinh B = g^b % p
    std::cout<<A<<" "<<B<<std::endl;

    // 4. Tinh bi mat chung
    BigInt512 alice_shared_secret = modular_exponentiation(B, a, p);    // Alice tinh s = B^a % p
    BigInt512 bob_shared_secret = modular_exponentiation(A, b, p);    // Bob tinh s = A^b % p
    std::cout<<alice_shared_secret<<" "<<bob_shared_secret<<std::endl;

    // 5. Hien thi ket qua va xac minh rang bi mat chung trung khop
    std::cout << "Bi mat chung Alice nhan duoc: " << alice_shared_secret << "\n";
    std::cout << "Bi mat chung Bob nhan duoc: " << bob_shared_secret << "\n";
    std::cout << "Qua trinh tinh toan dung khong? ";
    if (alice_shared_secret == bob_shared_secret)
        std::cout << "Dung.\n";
    else
        std::cout << "Sai.\n";

    return 0;
}





