#pragma once

#include "strawpoll_generated.h"

#include <array>
#include <cstdint>
#include <vector>
#include <unordered_set>
#include <iostream> // tmp debug


// Utility to construct std::array "literals" filled with given values
template <typename V, size_t N>
constexpr auto array_of(V v) {
  std::array<V, N> arr;
  arr.fill(v);
  return std::move(arr);
}
//==============================================================================
struct FlatBufferRef
{
  using ptr_t = const uint8_t*;

  const ptr_t  data;
  const size_t size;

  constexpr FlatBufferRef(ptr_t d, size_t s) noexcept : data(d), size(s) {}
};
//==============================================================================
struct PollDetail
{
  static constexpr auto TITLE = "When will C++ become obsolete?";
  static constexpr std::array<const char*, 5> OPTIONS = {
    "Around 2050",
    "Once all the cool kids use ...",
    "Never",
    "AI has no use for high level abstractions",
    "Turnip"
  };
};
// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
template<typename VOTE_GUARD_T>
class PollData : private PollDetail
{
  //----------------------------------------------------------------------------
  class FlatBufferWrapper
  {
  public:
    template<typename Func, 
             typename = std::enable_if_t<!std::is_same_v<std::decay_t<Func>, FlatBufferWrapper>>> 
    explicit FlatBufferWrapper(Func&& func)
    {
      flatbuffers::FlatBufferBuilder builder;
      func(builder);
      m_buffer = builder.Release();
    }
    FlatBufferWrapper(FlatBufferWrapper&&) = default;
    ~FlatBufferWrapper()                   = default;
    FlatBufferWrapper& operator =(FlatBufferWrapper&& othr) = default;

    FlatBufferWrapper            (const FlatBufferWrapper&) = delete;
    FlatBufferWrapper& operator =(const FlatBufferWrapper&) = delete;


    constexpr FlatBufferRef ref() const noexcept { return { m_buffer.data(), m_buffer.size() }; }

    // Copy new buffer into same area as the previous one to never invalidate PollData::m_result
    void inplace_assign(const FlatBufferWrapper& other)
    {
        if (other.m_buffer.size() == m_buffer.size()) {
          std::copy_n(other.m_buffer.data(), other.m_buffer.size(), m_buffer.data());
        } else {
          std::cerr << "inplace_assign() size mismatch!\n";
        }
    }

  private:
    flatbuffers::DetachedBuffer m_buffer;
  };
  //----------------------------------------------------------------------------
  static constexpr size_t N_OPTIONS = OPTIONS.size();
      
  using vote_t = int64_t;
  using votes_t = std::array<vote_t, N_OPTIONS>;

public:
  explicit PollData() : m_result{ make_result(array_of<vote_t, N_OPTIONS>(0)) } {
    m_votes.fill(0); // since std::array<primitive type,...> is uninitialized
  }
  PollData            (PollData&&) = default;
  PollData& operator= (PollData&&) = default;
  ~PollData()                      = default;

  PollData            (const PollData&) = delete;
  PollData& operator =(const PollData&) = delete;


  FlatBufferRef poll_response() const { return m_poll_response.ref(); }
  // . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
  template<typename FF, typename SF> 
  void register_vote(const vote_t vote,
                     const typename VOTE_GUARD_T::address_t& address,
                     FF&& fail_func,
                     SF&& success_func)
  {
    if (vote < 0 || vote > static_cast<vote_t>(OPTIONS.size()))  {
      fail_func(error_responses.invalid_vote);
    } else if (m_vote_guard.register_address(address)) {
      ++m_votes[vote];
      m_result.inplace_assign(make_result(m_votes));
      success_func(m_result.ref());
    } else { // already voted => simply show the results
      fail_func(m_result.ref());
    }
  }
  // . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
  struct ErrorResponses {
    const FlatBufferRef invalid_message = make_msg("Invalid request message");
    const FlatBufferRef invalid_type    = make_msg("Invalid request type");
    const FlatBufferRef invalid_vote    = make_msg("Invalid vote");
  };
  const ErrorResponses error_responses;
  // . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

private:
  template<typename T, size_t N>
  FlatBufferRef static make_msg(T(&msg)[N])
  {
    return FlatBufferWrapper{
      [&msg](flatbuffers::FlatBufferBuilder& fbb) -> void {
        const auto errorMsg = fbb.CreateString(msg);

        Strawpoll::ResponseBuilder res(fbb);
        res.add_type(Strawpoll::ResponseType_Error);
        res.add_error(errorMsg);
        fbb.Finish(res.Finish());
      }
    }.ref();
  }

  FlatBufferWrapper make_result(const votes_t& votes)
  {
    return FlatBufferWrapper{
      [&votes](flatbuffers::FlatBufferBuilder& fbb) -> void {
        const auto result = Strawpoll::CreateResult(fbb,
                                                    fbb.CreateVector(votes.data(), votes.size()));
        Strawpoll::ResponseBuilder res(fbb);
        res.add_type(Strawpoll::ResponseType_Result);
        res.add_result(result);
        fbb.Finish(res.Finish());
      }
    };
  }

//data:
  const FlatBufferWrapper m_poll_response {
    [](flatbuffers::FlatBufferBuilder& fbb) -> void {
      const auto poll = Strawpoll::CreatePoll(fbb,
                                              fbb.CreateString(TITLE),
                                              fbb.CreateVectorOfStrings([]{
                                                  std::vector<std::string> ret;
                                                  for (const auto& option : OPTIONS)
                                                    ret.emplace_back(option);
                                                  return ret;
                                              }()));
      Strawpoll::ResponseBuilder res(fbb);
      res.add_type(Strawpoll::ResponseType_Poll);
      res.add_poll(poll);
      fbb.Finish(res.Finish());
    }
  };
  VOTE_GUARD_T      m_vote_guard;
  votes_t           m_votes;
  FlatBufferWrapper m_result;
}; // class PollData
//==============================================================================
#if defined(CHECK_IP)
template<typename ADDRESS_T, template <typename> class H>
class VoteGuard
{
public:    
  using address_t = ADDRESS_T;

  VoteGuard()                              = default;
  VoteGuard(const VoteGuard& )             = default;
  VoteGuard(      VoteGuard&&)             = default;
  VoteGuard& operator =(const VoteGuard& ) = default;
  VoteGuard& operator =(      VoteGuard&&) = default;
  ~VoteGuard()                             = default;

  bool register_address(const ADDRESS_T& address) {
    return m_addresses.insert(address).second; // return false, if known address
  }

  bool has_voted(const ADDRESS_T& address) const {
    return m_addresses.count(address) != 0;
  }

private:
  std::unordered_set<ADDRESS_T, H<ADDRESS_T>> m_addresses;
};
#endif

struct EmptyAddress {};

template<typename ADDRESS_T> struct VoteGuardNoCheck
{
  using address_t = ADDRESS_T;

  bool register_address(const ADDRESS_T&)       { return true;  }
  bool has_voted       (const ADDRESS_T&) const { return false; }
};
