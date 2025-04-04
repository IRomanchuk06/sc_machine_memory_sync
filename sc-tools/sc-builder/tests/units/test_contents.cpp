/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "builder_test.hpp"

#include <sc-memory/sc_link.hpp>

TEST_F(ScBuilderTest, RelativeFile)
{
  /*
    "file://file.txt" => nrel_system_identifier:
      [test_content_file];;
   */

  LoadKB(m_ctx, {"contents.scs"});

  ScAddr const linkAddr = m_ctx->ResolveElementSystemIdentifier("test_content_file");
  EXPECT_TRUE(linkAddr.IsValid());

  ScLink link(*m_ctx, linkAddr);
  std::string const content = link.GetAsString();
  EXPECT_EQ(content, "file");
}

TEST_F(ScBuilderTest, RelativeFolder)
{
  LoadKB(m_ctx, {"contents.scs"});

  ScAddr const linkAddr = m_ctx->ResolveElementSystemIdentifier("test_content_file_2");
  EXPECT_TRUE(linkAddr.IsValid());

  ScLink link(*m_ctx, linkAddr);
  std::string const content = link.GetAsString();
  EXPECT_EQ(content, "contents/file");
}

TEST_F(ScBuilderTest, RelativeFolderSCsLevel1)
{
  LoadKB(m_ctx, {"contents.scs"});

  ScAddr const linkAddr = m_ctx->ResolveElementSystemIdentifier("test_content_file_3");
  EXPECT_TRUE(linkAddr.IsValid());

  ScLink link(*m_ctx, linkAddr);
  std::string const content = link.GetAsString();
  EXPECT_EQ(content, "contents/file");
}

template <typename ValueT>
void CheckBinaryContent(ScMemoryContext & ctx, std::string const & sysIdtf, ValueT value)
{
  ScAddr const linkAddr = ctx.ResolveElementSystemIdentifier(sysIdtf);
  EXPECT_TRUE(linkAddr.IsValid());

  ScLink link(ctx, linkAddr);
  EXPECT_TRUE(link.IsType<ValueT>());

  EXPECT_EQ(link.Get<ValueT>(), value);
}

TEST_F(ScBuilderTest, BinaryContents)
{
  LoadKB(m_ctx, {"contents.scs"});

  CheckBinaryContent<std::string>(*m_ctx, "test_content_string", "string");
  CheckBinaryContent<float>(*m_ctx, "test_content_float", 43.567f);
  CheckBinaryContent<double>(*m_ctx, "test_content_double", 543.345);
  CheckBinaryContent<int8_t>(*m_ctx, "test_content_int8", 8);
  CheckBinaryContent<int16_t>(*m_ctx, "test_content_int16", 16);
  CheckBinaryContent<int32_t>(*m_ctx, "test_content_int32", 32);
  CheckBinaryContent<int64_t>(*m_ctx, "test_content_int64", 64);
  CheckBinaryContent<uint8_t>(*m_ctx, "test_content_uint8", 108);
  CheckBinaryContent<uint16_t>(*m_ctx, "test_content_uint16", 116);
  CheckBinaryContent<uint32_t>(*m_ctx, "test_content_uint32", 132);
  CheckBinaryContent<uint64_t>(*m_ctx, "test_content_uint64", 164);
}
