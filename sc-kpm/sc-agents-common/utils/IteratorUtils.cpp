/*
* This source file is part of an OSTIS project. For the latest info, see http://ostis.net
* Distributed under the MIT License
* (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
*/

#include "IteratorUtils.hpp"
#include "CommonUtils.hpp"

using namespace std;

namespace utils
{

ScAddr IteratorUtils::getFirstFromSet(ScMemoryContext * ms_context, ScAddr & set)
{
  ScAddr element;
  ScIterator3Ptr iterator3 = ms_context->Iterator3(set, ScType::EdgeAccessConstPosPerm, ScType::Node);
  while (iterator3->Next())
  {
    element = iterator3->Get(2);
    break;
  }
  return element;
}

vector<ScAddr> IteratorUtils::getAllWithType(ScMemoryContext * ms_context, ScAddr & set, ScType scType)
{
  vector<ScAddr> elementList;
  ScIterator3Ptr iterator3 = ms_context->Iterator3(set, ScType::EdgeAccessConstPosPerm, scType);
  while (iterator3->Next())
  {
    elementList.push_back(iterator3->Get(2));
  }
  return elementList;
}

vector<ScAddr> IteratorUtils::getAllByInRelation(ScMemoryContext * ms_context, ScAddr & node, ScAddr & relation)
{
  vector<ScAddr> elementList;
  ScIterator5Ptr iterator5 = IteratorUtils::getIterator5(ms_context, node, relation, false);
  while (iterator5->Next())
  {
    elementList.push_back(iterator5->Get(0));
  }
  return elementList;
}


ScAddr IteratorUtils::getFirstByInRelation(ScMemoryContext * ms_context, ScAddr & node, ScAddr & relation)
{
  ScAddr element;
  ScIterator5Ptr iterator5 = IteratorUtils::getIterator5(ms_context, node, relation, false);
  while (iterator5->Next())
  {
    element = iterator5->Get(0);
    break;
  }
  return element;
}

ScAddr IteratorUtils::getFirstByOutRelation(ScMemoryContext * ms_context, ScAddr & node, ScAddr & relation)
{
  ScAddr element;
  ScIterator5Ptr iterator5 = IteratorUtils::getIterator5(ms_context, node, relation);
  while (iterator5->Next())
  {
    element = iterator5->Get(2);
    break;
  }
  return element;
}

ScIterator5Ptr IteratorUtils::getIterator5(
      ScMemoryContext * ms_context,
      ScAddr & node,
      ScAddr & relation,
      bool nodeIsStart)
{
  bool isRole = CommonUtils::checkType(ms_context, relation, ScType::NodeConstRole);
  ScType arcType = isRole ? ScType::EdgeAccessConstPosPerm : ScType::EdgeDCommonConst;

  ScIterator5Ptr iterator5;
  if (nodeIsStart)
    iterator5 = ms_context->Iterator5(node, arcType, ScType::Unknown, ScType::EdgeAccessConstPosPerm, relation);
  else
    iterator5 = ms_context->Iterator5(ScType::Unknown, arcType, node, ScType::EdgeAccessConstPosPerm, relation);
  return iterator5;
}

bool IteratorUtils::addSetToOutline(ScMemoryContext * ms_context, ScAddr & set, ScAddr & outline)
{
  if (!set.IsValid() || !outline.IsValid())
    return false;

  ScIterator3Ptr iterator3 = ms_context->Iterator3(set, ScType::EdgeAccessConstPosPerm, ScType::Unknown);
  while (iterator3->Next())
  {
    ms_context->CreateEdge(ScType::EdgeAccessConstPosPerm, outline, iterator3->Get(1));
    ms_context->CreateEdge(ScType::EdgeAccessConstPosPerm, outline, iterator3->Get(2));
  }
  return true;
}

bool IteratorUtils::addNodeWithOutRelationToOutline(ScMemoryContext * ms_context, ScAddr & node, ScAddr & relation,
                                                    ScAddr & outline)
{
  if (!node.IsValid() || !relation.IsValid() || !outline.IsValid())
    return false;

  ScIterator5Ptr iterator5 = IteratorUtils::getIterator5(ms_context, node, relation);
  while (iterator5->Next())
  {
    for (int i = 1; i < 5; i++)
      ms_context->CreateEdge(ScType::EdgeAccessConstPosPerm, outline, iterator5->Get(i));
  }
  return true;
}
}
