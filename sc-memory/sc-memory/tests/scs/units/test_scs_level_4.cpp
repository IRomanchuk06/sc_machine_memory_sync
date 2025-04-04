/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include <gtest/gtest.h>

#include "test_scs_utils.hpp"

TEST(scs_level_4, simple_1)
{
  char const * data = "a -> b: c; d;;";

  scs::Parser parser;
  EXPECT_TRUE(parser.Parse(data));

  TripleTester tester(parser);
  tester(
      {{{ScType::ConstNode, "a"}, {ScType::ConstPermPosArc, scs::Visibility::Local}, {ScType::ConstNode, "c"}},
       {{ScType::ConstNode, "b"},
        {ScType::ConstPermPosArc, scs::Visibility::Local},
        {ScType::ConstPermPosArc, scs::Visibility::Local}},
       {{ScType::ConstNode, "a"}, {ScType::ConstPermPosArc, scs::Visibility::Local}, {ScType::ConstNode, "d"}},
       {{ScType::ConstNode, "b"},
        {ScType::ConstPermPosArc, scs::Visibility::Local},
        {ScType::ConstPermPosArc, scs::Visibility::Local}}});

  auto const & triples = parser.GetParsedTriples();

  EXPECT_EQ(triples.size(), 4u);

  EXPECT_EQ(triples[1].m_target, triples[0].m_connector);
  EXPECT_EQ(triples[3].m_target, triples[2].m_connector);
}

TEST(scs_level_4, simple_2)
{
  char const * data = "a -> b: c; <- d: e: f;;";

  scs::Parser parser;
  EXPECT_TRUE(parser.Parse(data));

  TripleTester tester(parser);
  tester(
      {{{ScType::ConstNode, "a"}, {ScType::ConstPermPosArc, scs::Visibility::Local}, {ScType::ConstNode, "c"}},
       {
           {ScType::ConstNode, "b"},
           {ScType::ConstPermPosArc, scs::Visibility::Local},
           {ScType::ConstPermPosArc, scs::Visibility::Local},
       },
       {{ScType::ConstNode, "f"}, {ScType::ConstPermPosArc, scs::Visibility::Local}, {ScType::ConstNode, "a"}},
       {
           {ScType::ConstNode, "d"},
           {ScType::ConstPermPosArc, scs::Visibility::Local},
           {ScType::ConstPermPosArc, scs::Visibility::Local},
       },
       {
           {ScType::ConstNode, "e"},
           {ScType::ConstPermPosArc, scs::Visibility::Local},
           {ScType::ConstPermPosArc, scs::Visibility::Local},
       }});

  auto const & triples = parser.GetParsedTriples();

  EXPECT_EQ(triples.size(), 5u);

  EXPECT_EQ(triples[0].m_connector, triples[1].m_target);
  EXPECT_EQ(triples[2].m_connector, triples[3].m_target);
  EXPECT_EQ(triples[2].m_connector, triples[4].m_target);
}

TEST(scs_level_4, vector_simple_1)
{
  char const * data = "set1 -> <_el1; el2>;;";

  scs::Parser parser;
  EXPECT_TRUE(parser.Parse(data));

  TripleTester tester(parser);
  tester({
      {
          {ScType::ConstNodeTuple, scs::Visibility::Local},
          {ScType::ConstPermPosArc, scs::Visibility::Local},
          {ScType::VarNode, "_el1"},
      },
      {{ScType::ConstNode, "rrel_1"},
       {ScType::ConstPermPosArc, scs::Visibility::Local},
       {ScType::ConstPermPosArc, scs::Visibility::Local}},
      {
          {ScType::ConstNodeTuple, scs::Visibility::Local},
          {ScType::ConstPermPosArc, scs::Visibility::Local},
          {ScType::ConstNode, "el2"},
      },
      {{ScType::ConstPermPosArc, scs::Visibility::Local},
       {ScType::ConstCommonArc, scs::Visibility::Local},
       {ScType::ConstPermPosArc, scs::Visibility::Local}},
      {{ScType::ConstNode, "nrel_basic_sequence"},
       {ScType::ConstPermPosArc, scs::Visibility::Local},
       {ScType::ConstCommonArc, scs::Visibility::Local}},
      {{ScType::ConstNode, "set1"},
       {ScType::ConstPermPosArc, scs::Visibility::Local},
       {ScType::ConstNodeTuple, scs::Visibility::Local}},
  });

  auto const & triples = parser.GetParsedTriples();

  EXPECT_EQ(triples.size(), 6u);

  EXPECT_EQ(triples[1].m_target, triples[0].m_connector);
  EXPECT_EQ(triples[3].m_source, triples[0].m_connector);
  EXPECT_EQ(triples[3].m_target, triples[2].m_connector);
  EXPECT_EQ(triples[4].m_target, triples[3].m_connector);
  EXPECT_EQ(triples[5].m_target, triples[0].m_source);
}

TEST(scs_level_4, vector_simple_2)
{
  char const * data = "set1 -> <_el1>;;";

  scs::Parser parser;
  EXPECT_TRUE(parser.Parse(data));

  TripleTester tester(parser);
  tester({
      {
          {ScType::ConstNodeTuple, scs::Visibility::Local},
          {ScType::ConstPermPosArc, scs::Visibility::Local},
          {ScType::VarNode, "_el1"},
      },
      {{ScType::ConstNode, "rrel_1"},
       {ScType::ConstPermPosArc, scs::Visibility::Local},
       {ScType::ConstPermPosArc, scs::Visibility::Local}},
      {{ScType::ConstNode, "set1"},
       {ScType::ConstPermPosArc, scs::Visibility::Local},
       {ScType::ConstNodeTuple, scs::Visibility::Local}},
  });

  auto const & triples = parser.GetParsedTriples();

  EXPECT_EQ(triples.size(), 3u);

  EXPECT_EQ(triples[1].m_target, triples[0].m_connector);
  EXPECT_EQ(triples[2].m_target, triples[0].m_source);
}

TEST(scs_level_4, vector_simple_3)
{
  char const * data = "set1 -> <>;;";

  scs::Parser parser;
  EXPECT_FALSE(parser.Parse(data));
}

TEST(scs_level_4, vector_4)
{
  char const * data = "set1 -> <<_el1; el3>; el2>;;";

  scs::Parser parser;
  EXPECT_TRUE(parser.Parse(data));

  TripleTester tester(parser);
  tester({
      {
          {ScType::ConstNodeTuple, scs::Visibility::Local},
          {ScType::ConstPermPosArc, scs::Visibility::Local},
          {ScType::VarNode, "_el1"},
      },
      {{ScType::ConstNode, "rrel_1"},
       {ScType::ConstPermPosArc, scs::Visibility::Local},
       {ScType::ConstPermPosArc, scs::Visibility::Local}},
      {
          {ScType::ConstNodeTuple, scs::Visibility::Local},
          {ScType::ConstPermPosArc, scs::Visibility::Local},
          {ScType::ConstNode, "el3"},
      },
      {{ScType::ConstPermPosArc, scs::Visibility::Local},
       {ScType::ConstCommonArc, scs::Visibility::Local},
       {ScType::ConstPermPosArc, scs::Visibility::Local}},
      {{ScType::ConstNode, "nrel_basic_sequence"},
       {ScType::ConstPermPosArc, scs::Visibility::Local},
       {ScType::ConstCommonArc, scs::Visibility::Local}},
      {{ScType::ConstNodeTuple, scs::Visibility::Local},
       {ScType::ConstPermPosArc, scs::Visibility::Local},
       {ScType::ConstNodeTuple, scs::Visibility::Local}},
      {{ScType::ConstNode, "rrel_1"},
       {ScType::ConstPermPosArc, scs::Visibility::Local},
       {ScType::ConstPermPosArc, scs::Visibility::Local}},
      {
          {ScType::ConstNodeTuple, scs::Visibility::Local},
          {ScType::ConstPermPosArc, scs::Visibility::Local},
          {ScType::ConstNode, "el2"},
      },
      {{ScType::ConstPermPosArc, scs::Visibility::Local},
       {ScType::ConstCommonArc, scs::Visibility::Local},
       {ScType::ConstPermPosArc, scs::Visibility::Local}},
      {{ScType::ConstNode, "nrel_basic_sequence"},
       {ScType::ConstPermPosArc, scs::Visibility::Local},
       {ScType::ConstCommonArc, scs::Visibility::Local}},
      {{ScType::ConstNode, "set1"},
       {ScType::ConstPermPosArc, scs::Visibility::Local},
       {ScType::ConstNodeTuple, scs::Visibility::Local}},
  });

  auto const & triples = parser.GetParsedTriples();

  EXPECT_EQ(triples.size(), 11u);

  EXPECT_EQ(triples[1].m_target, triples[0].m_connector);
  EXPECT_EQ(triples[3].m_source, triples[0].m_connector);
  EXPECT_EQ(triples[3].m_target, triples[2].m_connector);
  EXPECT_EQ(triples[4].m_target, triples[3].m_connector);
  EXPECT_EQ(triples[5].m_target, triples[0].m_source);
  EXPECT_EQ(triples[6].m_target, triples[5].m_connector);
  EXPECT_EQ(triples[8].m_source, triples[5].m_connector);
  EXPECT_EQ(triples[8].m_target, triples[7].m_connector);
  EXPECT_EQ(triples[9].m_target, triples[8].m_connector);
  EXPECT_EQ(triples[10].m_target, triples[5].m_source);
}
