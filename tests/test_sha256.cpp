// The hash every save is checked with: the published SHA-256 answers, the
// padding at the lengths where it spills into a second block, the same digest
// whether the bytes arrive at once or in pieces, and finish() leaving the
// hasher ready for the next message.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "check.h"
#include "save/sha256.h"

using namespace sisyphus;

namespace {
std::string hashOf(const std::string& s) { return toHex(sha256(s.data(), s.size())); }

// The same bytes fed `step` at a time.
std::string hashInSteps(const std::string& s, size_t step) {
    Sha256 h;
    for (size_t i = 0; i < s.size(); i += step) h.update(s.data() + i, s.size() - i < step ? s.size() - i : step);
    return toHex(h.finish());
}
}  // namespace

int main() {
    // The answers published with the standard (FIPS 180-2, appendix B).
    CHECK(hashOf("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    CHECK(hashOf("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    const std::string twoBlocks = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    CHECK(hashOf(twoBlocks) == "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    const std::string million(1000000, 'a');
    CHECK(hashOf(million) == "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");

    // Either side of where the padding and length no longer fit in one block
    // (55 and 56 bytes) or fill it exactly (64), and the same a block later.
    CHECK(hashOf(std::string(55, 'a')) == "9f4390f8d30c2dd92ec9f095b65e2b9ae9b0a925a5258e241c9f1e910f734318");
    CHECK(hashOf(std::string(56, 'a')) == "b35439a4ac6f0948b6d6f9e3c6af0f5f590ce20f1bde7090ef7970686ec6738a");
    CHECK(hashOf(std::string(63, 'a')) == "7d3e74a05d7db15bce4ad9ec0658ea98e3f06eeecf16b4c6fff2da457ddc2f34");
    CHECK(hashOf(std::string(64, 'a')) == "ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb");
    CHECK(hashOf(std::string(119, 'a')) == "31eba51c313a5c08226adf18d4a359cfdfd8d2e816b13f4af952f7ea6584dcfb");
    CHECK(hashOf(std::string(120, 'a')) == "2f3d335432c70b580af0e8e1b3674a7c020d683aa5f73aaaedfdc55af904c21c");

    // Every byte value, three times over: the same digest however the bytes
    // are split between update() calls.
    std::string all;
    for (int r = 0; r < 3; ++r)
        for (int b = 0; b < 256; ++b) all += static_cast<char>(static_cast<unsigned char>(b));
    const std::string allHex = "f3a25aa93aa2fbba28d79260535bbd6a5eb0fc1c24a8b0f04e12b484c1dfe363";
    CHECK(hashOf(all) == allHex);
    for (size_t step : {size_t{1}, size_t{7}, size_t{63}, size_t{64}, size_t{65}, size_t{500}})
        CHECK(hashInSteps(all, step) == allHex);
    CHECK(hashInSteps(million, 4093) == "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");

    // finish() resets: the next message hashes as if the hasher were new.
    Sha256 h;
    h.update(twoBlocks.data(), twoBlocks.size());
    const std::array<uint8_t, 32> first = h.finish();
    h.update("abc", 3);
    CHECK(toHex(h.finish()) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CHECK(toHex(h.finish()) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    // reset() throws away what was fed so far.
    h.update("xyz", 3);
    h.reset();
    h.update("abc", 3);
    CHECK(toHex(h.finish()) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    // toHex writes every byte as two lowercase digits, most significant first.
    CHECK(first[0] == 0x24 && first[31] == 0xc1);
    std::array<uint8_t, 32> d{};
    d[0] = 0x0f;
    d[1] = 0xa0;
    d[31] = 0xff;
    CHECK(toHex(d) == "0fa0" + std::string(58, '0') + "ff");
    TEST_MAIN_END();
}
