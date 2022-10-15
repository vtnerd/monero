

namespace cryptonote
{
  namespace
  {
    template<typename F, typename T>
    void txin_gen_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(height));
    }

    template<typename F, typename T>
    void txin_to_script_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(prev), WIRE_FIELD(prevout), WIRE_FIELD(sigset));
    }
    
    template<typename F, typename T>
    void txin_to_scripthash_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(prev), WIRE_FIELD(prevout), WIRE_FIELD(script), WIRE_FIELD(sigset));
    }

    template<typename F, typename T>
    void txin_to_key_map(F& format, T& self)
    {
      wire::object(format,
        WIRE_FIELD(amount),
        WIRE_FIELD(key_offsets),
        wire::field("key_image", std::ref(self.k_image))
      );
    }
  } // anonymous
  WIRE_DECLARE_OBJECT(txin_gen, txin_gen_map)
  WIRE_DECLARE_OBJECT(txin_to_script, txin_to_script_map)
  WIRE_DECLARE_OBJECT(txin_to_scripthash, txin_to_scripthash_map)
  WIRE_DECLARE_OBJECT(txin_to_key, txin_to_key)

  namespace
  {
    template<typename F, typename T>
    void txin_gen_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(height));
    }

    template<typename F, typename T>
    void txout_to_script_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(keys), WIRE_FIELD(script));
    }

    template<typename F, typename T>
    void txout_to_scripthash_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(hash));
    }

    template<typename F, typename T>
    void txout_to_key_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(key));
    }
    
    template<typename F, typename T>
    void txout_to_tagged_key_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(key), WIRE_FIELD(view_tag));
    }

    template<typename F, typename T>
    void tx_out_map(F& format, T& self)
    {
      wire::object(format,
        WIRE_FIELD(amount),
        WIRE_VARIANT_OPTION("to_key", target, txout_to_key),
        WIRE_VARIANT_OPTION("to_tagged_key", target, txout_to_tagged_key),
        WIRE_VARIANT_OPTION("to_script", target, txout_to_script),
        WIRE_VARIANT_OPTION("to_scripthash", target, txout_to_scripthash)
      );                           
    }
  } // anonymous
  WIRE_DECLARE_OBJECT(txout_to_script, txout_to_script_map)
  WIRE_DECLARE_OBJECT(txout_to_scripthash, txout_to_scripthash_map)
  WIRE_DECLARE_OBJECT(txout_to_key, txout_to_key_map)
  WIRE_DECLARE_OBJECT(txout_to_tagged_key, txout_to_tagged_key)
  WIRE_DECLARE_OBJECT(tx_out, tx_out_map)
} // cryptonote

namespace rct
{
  namespace
  {
    template<typename F, typename T>
    void range_sig_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(asig));
    }

    template<typename F, typename T>
    void bulletproof_map(F& format, T& self)
    {
      wire::object(format,
        WIRE_FIELD(V),
        WIRE_FIELD(A),
        WIRE_FIELD(S),
        WIRE_FIELD(T1),
        WIRE_FIELD(T2),
        WIRE_FIELD(taux),
        WIRE_FIELD(mu),
        WIRE_FIELD(L),
        WIRE_FIELD(R),
        WIRE_FIELD(a),
        WIRE_FIELD(b),
        WIRE_FIELD(t)
      );
    }

    template<typename F, typename T>
    void bulletproof_plus_map(F& format, T& self)
    {
      wire::object(format,
        WIRE_FIELD(V),
        WIRE_FIELD(A),
        WIRE_FIELD(A1),
        WIRE_FIELD(B),
        WIRE_FIELD(r1),
        WIRE_FIELD(s1),
        WIRE_FIELD(d1),
        WIRE_FIELD(L),
        WIRE_FIELD(R)
      );
    }

    template<typename F, typename T, typename U>
    void boro_sig_map(F& format, T& self, U&& s0, U&& s1)
    {
      wire::object(format, WIRE_FIELD(ee), wire::field("s0", std::ref(s0)), wire::field("s1", std::ref(s1)));
    }

    template<typename F, typename T>
    void mg_sig_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(ss), WIRE_FIELD(cc));
    }
    
    template<typename F, typename T>
    void rct_sig_prunable(F& format, T& self)
    {
      using range_proof_max = wire::max_element_count<256>;
      using bulletproof_max = wire::max_element_count<BULLETPROOF_MAX_OUTPUTS>;
      using bulletproof_plus_max = wire::max_element_COUNT<BULLETPROOF_PLUS_MAX_OUTPUTS>;
      using mlsags_max = wire::max_element_count<256>;
      using pseudo_outs_max = wire::max_element_count<256>;

      // make all arrays required for backwards compatability
      wire::object(format,
        wire::field("range_proofs", wire::array<range_proof_max>(std::ref(self.rangeSigs))),
        wire::field("bulletproofs", wire::array<bulletproof_max>(std::ref(self.bulletproofs)),
        wire::field("bulletproof_plus", wire::array<bulletproof_plus_max>(std::ref(self.bulletproofs_plus))),
        wire::field("mlsags", wire::array<mlsags_max>(std::ref(self.MGs))),
        wire::field("pseudo_outs", wire::array<pseudo_outs_max>(std::ref(self.get_pseudo_outs())))
      );
    }

    template<typename F, typename T>
    void ecdh_tuple_map(F& format, T& self)
    {
      wire::object(format, WIRE_FIELD(mask), WIRE_FIELD(amount));
    }
    
    template<typename F, typename T>
    void rct_sig_map(F& format, T& self)
    {
      using min_commitment_size = wire::min_element_sizeof<rct::key>;
      wire::object(format,
        WIRE_FIELD(type),
        wire::optional_field("encrypted", std::ref(encrypted)),
        wire::optional_field("commitments", wire::array<min_commitment_size>(std::ref(self.outPk))),
        wire::optional_field("fee", std::ref(fee)),
        wire::optional_field("prunable", std::ref(prunable))
      );
    }
  } // anonymous

  WIRE_DEFINE_OBJECT(rangeSig, range_sig_map)
  WIRE_DEFINE_OBJECT(Bulletproof, bulletproof_map)
  WIRE_DEFINE_OBJECT(BulletproofPlus, bulletproof_plus_map)
  WIRE_DEFINE_OBJECT(ecdhTuple, ecdh_tuple_map)

  void read_bytes(wire::reader& source, rct::boroSig& dest)
  {
    boost::container::static_vector<rct::key, 64> s0;
    boost::container::static_vector<rct::key, 64> s1;
    boro_sig_map(source, dest, s0, s1);

    if (s0.size() != boost::size(dest.s0) || s1.size() != boost::size(dest.s1))
      WIRE_DLOG_THROW(wire::error::schema::array, "invalid array size");

    boost::range::copy(s0, boost::begin(dest.s0));
    boost::range::copy(s1, boost::begin(dest.s1));
  }
  void write_bytes(wire::writer& dest, const rct::boroSig& source)
  {
    using key_span = epee::span<const rct::key>;
    boro_sig_map(dest, source, key_span{source.s0}, key_span{source.s1});
  }

  void read_bytes(wire::reader& source, rctSig& dest)
  {
    boost::optional<rct::ecdhTuple&> encrypted;
    boost::optional<rct::xmr_amount&> fee;
    boost::optional<rct::rctSigPrunable&> prunable;

    encrypted.emplace(dest.ecdhInfo);
    fee.emplace(dest.txnFee);
    prunable.emplace(dest.p);

    rct_sig_map(source, dest, encrypted, fee, prunable);
    if (dest.type == rct::RCTTypeNull && (fee || encrypted))
      WIRE_DLOG_THROW(wire::error::schema::missing_key, "unexpected keys");
    if (dest.type != rct::RCTTypeNull && (!fee || !encrypted))
      WIRE_DLOG_THROW(wire::error::schema::missing_key, "expected keys");

    if (!prunable)
    {
      dest.p.rangeSigs.clear();
      dest.p.bulletproofs.clear();
      dest.p.bulletproofs_plus.clear();
      dest.p.MGs.clear();
      dest.p.get_pseudo_outs().clear();
    }
  }
  void write_bytes(wire::writer& dest, const rctSig& source)
  {
    boost::optional<const rct::ecdhTuple&> encrypted;
    boost::optional<rct::xmr_amount> fee;
    boost::optional<const rctSigPrunable&> prunable;
    
    if (self.type != rct::RCTTypeNull)
    {
      encrypted.emplace(self.ecdhInfo);
      fee.emplace(self.txnFee);
    }
    
    if (!source.p.bulletproofs.empty() || !sig.p.bulletproofs_plus.empty() || !sig.p.rangeSigs.empty() || !sig.get_pseudo_outs().empty())
      prunable.emplace(source.p);

    rct_sig_map(dest, source, encrypted, fee, prunable);
  }

  WIRE_DEFINE_OBJECT(rctSigPrunable, rct_sig_prunable_map)
  WIRE_DEFINE_OBJECT(rctSig, rct_sig_map)
} // rct
