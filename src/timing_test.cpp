
#include <boost/utility/string_ref.hpp>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "byte_slice.h"
#include "byte_stream.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "hex.h"

constexpr const std::string_view tx_hex{"020002020010b6b59403c8ba02adc107eb8902e516669c1ec82176ba058a0132e80151757264858eabe20528e88f2b996c0822faf38f1c50917e59df3071447219361029a2020010cad6bd02c3d260c75d9b9401f8349a0d9902f80dd51bd4093cc9077ecf01715a4599a579c69f2af67ec411b0ba37a1ed072677491661424292664306ce4e42670200034711b6bfc2a17d0c3fd3c04e9e29545aaeae1859c4f1788515c555e81f51ec284700030afadbfa893d7001e1c0192bad076cff4494a4b46b5a43c70c6102cba57df275bb2c0134c5d300ef1e75a811b7eb8297c84ef463cd51039363827f182a294d1e4878e0020901e0a468ededfa47880680d5f81465e7c2043e50bd02018a6cefd7f1f3780b1e0cd759462eb07c24d24b43f51dafdd2ab3613c2ef156961291f0ac5bbc75002e380e505a4578f1e81db9f87bb81b6820094850c09dbd0f9a1d32bc7799ac013f46da2a53f1870579d15b7c80f3679a2b6a7635c496db727d99f7fb0f632e7ce93f6a650b99e95a5a60e56a51f636f6c29dd17e28314477a63edd3520630ce13e0664606a10b03a63bf2eb70f3e34b57981a40435644102fdd56a271afb6fe488e9e2dbe295fe1a69657f9e3bf96cba216a21c9e0c4a373b60723fd56d4c8089cca232ab43dd642ce3cf68a36d345010eb28d977dffa3fff2609b2c8c6b5103acf73fdace49a7dfb5cbff8fb6b14eefec661569273ae18d4af4eec659eaa304074a776a7fa37de611724865d9dd1c48ffe258f5a927d80175fa4fc623827c89104ba0bde442005b2df028e3d970b790246612159b91a1b4bfdcf0a4f411ef068434734b6f6eae7480705bb1650099598d24276b2e5f7ad00ee3d5f3f0e342bda245df2b662c4032b1958751df2aa34029cf12b48fbaddcec026bda4930d453c17e560d604be0259766e62d0966abddbeb93fff1acd84da7e60badee0506c33d71969e0af39dbc45c23edc9152284938a35ccc601435559bc9c741188ebebc5bfa80dcd2bcdaacecc05bb631b7abda3be5d65500a86f4ced84ea543193c9bdf3f60718246483219890520ae390f82d620a434b2069019676bc64fa1a0d064dbf6aa1d311d55aaadec71655038e996c3613ad3b7f71f45f02dae4e8728a9a46f73aa0dbbf7e9ec7fe159a4f6b4fb59126d900546e8aecf5be430dcbf2e32008ab94b9ac80862d34a92abbe08183ac42122aaa0f420ee6cb20e5e85a370d3c1bb66e8d962f35f4bd4223082dd6759024c18a52e7dd158beb0b4ead950e11bdb6a385699b362ee7f9b49204bfdb811a99e66f0d635c73a608f5fc1b61552585b5c3095ed6037d49a75b0f60bc8b2e28aa0410da5b1d10a78818d41124984cd6ec6e147f157760134368b1a99fa6ba77b1b79eebaae06ff9cda4a82a38dec05ea836c608422ff5fdccbf155a8de19a11f304e72d99ee3904ba15916552f0da0e1c0ff90ecf18b4ce2cba6ebd18cb043229ad9864b4489fbc332dfc9060649e759ba1f00e25fa420af03db51c124538c3973eed590c7df7140a68017045c1bb22a7669006673810dee29f14e8abc6a34282bdc7f3182da4f8cdc13757059114159215980b90fa6dc2ffbf5781bfa4ca9b4685633c9b4e004d75ed0a00df6ad051d2b3af04b42923916cc01673cecb3097ceafc085561152ba73b4ccbc7a794189269a0a00f0219c3947e9427c5c91a207dc4708b2f513632119d17957e880849d199932011576ecb10633311cece2ed45fd28d499c4556c0b3675504e8bf0d05436d88e02c7aa2ceaf46721c196527e72044be09897309d22f8ee4de505fc0d12f959d406930eebc74b23608483471f9af731977c1e82dd0530d7e3532bb4d1b50d15070835594af9563ef49af429c49dd3868a82b56f826f8cc63c2c5e3e04d82f735a041d710771750088b1fba222e0e74fdb73c874ca51fa7cdfd88b18149ce3ccfb021ddd3d1b8c2e28e7900fe21ec0b7f07cc222d758fa4dccd07ebfadfeabb4c00c8d9275e104617c568b3f33eab464f8d2f56a72f6c613b09fcaf4c6d9441f5b08d90855870628fa6d650a9821009fe27900b562a0c0bbd2e3590a0f9dbda0f402bfe60c8747e33fb711c1b27a7af57aac872f811d460e0dbca73a1b107284480aedc6d49820ec99d9e50577182ffa8f75d321be65c2e2ce65fb9ed88b116c68845c63c60d1f141b06b40f1ff43f12c3bd9177b69321361b85799f2cdb19cc5708beb632c5fa1932f4ec9e60982d17ff69895704d6dc2d3a9cbdbde6b7d71c9a0377b56a81459195181ecd5e1330937e233117869062ee97cd049c1db865a186069a8b949334c4c170e07d6a7906c304249050bd603be50ff2223adf8ae429a80de42c487a39231290f12f2ecd6c7125f7d0f57329cfd9a6b2ea42115e1b125d017a18ef63dc5cd692fffbe0a2c5e9910d9a43723ab7673ef75ce141ed9b79400b644cae5b5d74e24112d270396b25306a81713b1fc3328248501863f4d819870561fc9ffc7100c1991d3b12aed46dbc118b9e49d32b9dce63276db0d6397f8e0683ca8a1640b3f0dd423c0940e68e32240c6adfe8dee58d3d5200933f53d5fa0bc155ef19cdc017a9b1a28d9e56706dc59ff5f9e1461e5a8a5164e5bde62e7f0a0bbc7b49e414b6a6af8dfd55c08ed8488d84a4586890c895ad7c6e3d7692ee08644cbf5bd1f94dd9a2dc499dd56fd00e1786b87bd4888a80f1bf2582df0c8c08580e00964d0468739c394a6f6d794d72feed9bc2cf44fae4ed1a47070390fc0ff6b5c332af650240f6de30b310a69b708774757f5ad08e4c94f5e58cd1ec530e8c6f9329206385be19feb53ea476f319abc1ff2c4c8a43cfe8ab5d87787a4803d87f15b32859a66682ee50da133ca9067335eca39fac25d59e9a95bb158e30325a6e137af922c15f322edb9ddce72ae6b6641e8a98c7c528a7182bba25943071f7e56976a940c81df8af9ff84dbd4128a0d84ddd13a673626706ebadcf7e1fbb1b964924fda552268accdb0b0f136126e10dba8ae3cdc11abc64f7e44ce0bd7215d089bdcf7330c2e30b7b954273c5a741214e0a2ed04afd4a896c9b5fe35bda"};

struct baseline
{
  static constexpr const char* name() noexcept { return "baseline"; }

  unsigned operator()(const cryptonote::transaction& tx, const epee::byte_slice&) const noexcept
  {
    return tx.vin.size();
  }
};

struct ref_copy
{
  static constexpr const char* name() noexcept { return "ref_copy"; }

  unsigned operator()(const cryptonote::transaction&, const epee::byte_slice& blob) const noexcept
  {
    return blob.clone().size();
  }
};

struct byte_copy
{
  static constexpr const char* name() noexcept { return "byte_copy"; }

  unsigned operator()(const cryptonote::transaction&, const epee::byte_slice& blob) const
  {
    return epee::byte_slice{{epee::to_span(blob)}}.size();
  }
};

struct tx_copy
{
  static constexpr const char* name() noexcept { return "tx_copy"; };

  unsigned operator()(const cryptonote::transaction& tx, const epee::byte_slice&) const
  {
    const cryptonote::transaction copy{tx};
    return copy.vin.size();
  }
};

struct tx_move
{
  static constexpr const char* name() noexcept { return "tx_move"; };

  unsigned operator()(cryptonote::transaction&& tx, const epee::byte_slice&) const
  {
    const cryptonote::transaction copy{std::move(tx)};
    return copy.vin.size();
  }
};

struct tx_unpack
{
  static constexpr const char* name() noexcept { return "tx_unpack"; };

  unsigned operator()(cryptonote::transaction&& tx, const epee::byte_slice& tx_blob) const
  {
    tx.set_null();
    return t_serializable_object_from_blob(tx, epee::to_span(tx_blob));
  }
};

template<typename F>
void runner(const epee::byte_slice& tx_blob, F f)
{
  unsigned state = 0;
  for (unsigned i = 0; i < 100; ++i)
  {
    cryptonote::transaction tx;
    if (!t_serializable_object_from_blob(tx, epee::to_span(tx_blob)))
      throw std::runtime_error{"unpack issue"};
    state += f(std::move(tx), tx_blob);
  }

  const auto start = std::chrono::steady_clock::now();
  for (unsigned i = 0; i < 1000; ++i)
  {
    cryptonote::transaction tx;
    if (!t_serializable_object_from_blob(tx, epee::to_span(tx_blob)))
      throw std::runtime_error{"unpack issue"};
    state += f(std::move(tx), tx_blob);
  }
  const auto end = std::chrono::steady_clock::now();
  std::cout << f.name() << " " << state << " "  << (end - start).count() << std::endl;
}

int main()
{
  epee::byte_slice tx_blob;
  {
    epee::byte_stream out;
    out.put_n(0, tx_hex.size() / 2);
    if (!epee::from_hex::to_buffer(epee::to_mut_span(out), {tx_hex.data(), tx_hex.size()}))
      throw std::runtime_error{"hex error"};
    tx_blob = epee::byte_slice{std::move(out)};
  }
  runner(tx_blob, baseline{});
  runner(tx_blob, ref_copy{});
  runner(tx_blob, byte_copy{});
  runner(tx_blob, tx_copy{});
  runner(tx_blob, tx_move{});
  runner(tx_blob, tx_unpack{});
  return 0;
}
