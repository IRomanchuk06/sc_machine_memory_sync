#include "sc_python_module.hpp"

#include "../sc_memory.hpp"
#include "../sc_stream.hpp"

namespace
{
namespace bp = boost::python;

void translateException(utils::ScException const & e)
{
  // Use the Python 'C' API to set up an exception object 
  PyErr_SetString(PyExc_RuntimeError, e.Message());
}

namespace impl
{

class PyTemplateGenResult
{
public:
  PyTemplateGenResult()
    : m_result(new ScTemplateGenResult())
  {
  }

  ScTemplateGenResult & GetResultRef()
  {
    return *m_result;
  }

  ScAddr Get(std::string const & name) const
  {
    return (*m_result)[name];
  }

  size_t Size() const
  {
    return m_result->Size();
  }

private:
  std::shared_ptr<ScTemplateGenResult> m_result;
};

// -----------------------------
class PyTemplateSearchResultItem
{
public:
  PyTemplateSearchResultItem()
    : m_item(new ScTemplateSearchResultItem(nullptr, nullptr))
  {
  }

  PyTemplateSearchResultItem(ScAddrVector const * results, ScTemplate::ReplacementsMap const * replacements)
    : m_item(new ScTemplateSearchResultItem(results, replacements))
  {
  }

  ScTemplateSearchResultItem & GetItemRef()
  {
    return *m_item;
  }

  ScAddr Get(bp::object & ind) const
  {
    bp::extract<std::string> se(ind);
    if (se.check())
      return (*m_item)[static_cast<std::string>(se)];

    bp::extract<int64_t> ie(ind);
    if (ie.check())
      return (*m_item)[static_cast<int64_t>(ie)];

    return ScAddr();
  }

  size_t Size() const
  {
    return m_item->Size();
  }

private:
  std::shared_ptr<ScTemplateSearchResultItem> m_item;
};

// -----------------------------
class PyTemplateSearchResult
{
public:
  PyTemplateSearchResult()
    : m_result(new ScTemplateSearchResult())
  {
  }

  ScTemplateSearchResult & GetResultRef()
  {
    return *m_result;
  }

  size_t Size() const
  {
    return m_result->Size();
  }

  bp::object Get(size_t idx) const
  {
    PyTemplateSearchResultItem result;
    if (m_result->GetResultItemSave(idx, result.GetItemRef()))
      return bp::object(result);

    return bp::object();
  }

private:
  std::shared_ptr<ScTemplateSearchResult> m_result;
};

// -----------------------------
class PyTemplateItemValue
{
public:
  PyTemplateItemValue() {}

  PyTemplateItemValue(ScAddr const & addr, std::string const & replName = std::string())
    : m_item(new ScTemplateItemValue(addr, replName.c_str()))
  {
  }

  PyTemplateItemValue(ScType const & type, std::string const & replName = std::string())
    : m_item(new ScTemplateItemValue(type, replName.c_str()))
  {
  }

  explicit PyTemplateItemValue(std::string const & name)
    : m_item(new ScTemplateItemValue(name))
  {
  }

  ScTemplateItemValue & GetItemRef() const
  {
    return *m_item;
  }

private:
  std::shared_ptr<ScTemplateItemValue> m_item;
};

// -----------------------------
bp::object _scAddrToRShift(ScAddr const & addr, std::string const & replName)
{
  return bp::object(PyTemplateItemValue(addr, replName));
}

bp::object _scTypeToRShift(ScType const & type, std::string const & replName)
{
  return bp::object(PyTemplateItemValue(type, replName));
}

// -----------------------------
class PyLinkContent
{
public:
  PyLinkContent() {}

  explicit PyLinkContent(ScStream const & stream)
  {
    m_buffer.reset(new MemoryBufferSafe());
    m_buffer->Reinit(stream.Size());
    size_t readBytes = 0;
    stream.Read((sc_char*)m_buffer->Data(), stream.Size(), readBytes);
  }

  std::string AsString() const
  {
    char const * data = (char const*)m_buffer->CData();
    return std::string(data, data + m_buffer->Size());
  }

  int64_t AsInt() const
  {
    if (m_buffer->Size() != sizeof(int64_t))
    {
      SC_THROW_EXCEPTION(utils::ExceptionInvalidType,
                         "Size of content should be equal to " << sizeof(int64_t) << " bytes");
    }

    int64_t value = 0;
    m_buffer->Read(&value, sizeof(value));
    return value;
  }

  double AsDouble() const
  {
    if (m_buffer->Size() != sizeof(double))
    {
      SC_THROW_EXCEPTION(utils::ExceptionInvalidType,
                         "Size of content should be equal to " << sizeof(double) << " bytes");
    }

    double value = 0.0;
    m_buffer->Read(&value, sizeof(value));
    return value;
  }

private:
  MemoryBufferSafePtr m_buffer;
};

bp::tuple _context_getEdgeInfo(ScMemoryContext & self, ScAddr const & addr)
{
  ScAddr src, trg;
  if (self.GetEdgeInfo(addr, src, trg))
    return bp::make_tuple(src, trg);

  return bp::make_tuple(bp::object(), bp::object());
}

template <typename T>
bool _set_contentT(ScMemoryContext & self, ScAddr const & linkAddr, bp::extract<T> & v)
{
  T const value = v;
  ScStreamPtr const stream = MakeReadStreamT(value);
  return self.SetLinkContent(linkAddr, *stream);
}

bool _context_setLinkContent(ScMemoryContext & self, ScAddr const & linkAddr, bp::object & content)
{
  if (content.is_none() || !linkAddr.IsValid())
    return false;

  bp::extract<int64_t> l(content);
  if (l.check())
    return _set_contentT(self, linkAddr, l);

  bp::extract<double> d(content);
  if (d.check())
    return _set_contentT(self, linkAddr, d);

  bp::extract<std::string> s(content);
  if (s.check())
  {
    std::string const value = s;
    ScStream stream(value.c_str(), value.size(), SC_STREAM_FLAG_READ);
    return self.SetLinkContent(linkAddr, stream);
  }

  return false;
}

bp::object _context_getLinkContent(ScMemoryContext & self, ScAddr const & linkAddr)
{
  ScStream stream;
  if (self.GetLinkContent(linkAddr, stream))
    return bp::object(PyLinkContent(stream));

  return bp::object();
}

template <typename TIteratorType>
class PyIteratorWrap
{
public:
  PyIteratorWrap()
  {
  }

  explicit PyIteratorWrap(TIteratorType const & iter)
    : m_iter(iter)
  {
  }

  bool IsValid() const
  {
    SC_ASSERT(m_iter.get(), ());
    return m_iter->IsValid();
  }

  bool Next() const
  {
    SC_ASSERT(m_iter.get(), ());
    return m_iter->Next();
  }

  ScAddr Get(uint8_t index) const
  {
    SC_ASSERT(m_iter.get(), ());
    return m_iter->Get(index);
  }

protected:
  TIteratorType m_iter;
};

using PyIterator3 = PyIteratorWrap<ScIterator3Ptr>;
using PyIterator5 = PyIteratorWrap<ScIterator5Ptr>;

bp::object _context_iterator3(ScMemoryContext & self,
                              bp::object & param1,
                              bp::object & param2,
                              bp::object & param3)
{
  bp::extract<ScType> pt2(param2);
  if (!pt2.check()) // second parameter should be a type anyway
    return bp::object();

  bp::extract<ScAddr> pa1(param1);
  if (pa1.check())
  {
    bp::extract<ScAddr> pa3(param3);
    if (pa3.check()) // f_a_f
      return bp::object(PyIterator3(self.Iterator3(static_cast<ScAddr>(pa1),
                                                   static_cast<ScType>(pt2),
                                                   static_cast<ScAddr>(pa3))));

    bp::extract<ScType> pt3(param3);
    if (pt3.check()) // f_a_a
      return bp::object(PyIterator3(self.Iterator3(static_cast<ScAddr>(pa1),
                                                   static_cast<ScType>(pt2),
                                                   static_cast<ScType>(pt3))));
  }

  bp::extract<ScType> pt1(param1);
  if (pt1.check())
  {
    bp::extract<ScAddr> pa3(param3);
    if (pa3.check()) // a_a_f
      return bp::object(PyIterator3(self.Iterator3(static_cast<ScType>(pt1),
                                                   static_cast<ScType>(pt2),
                                                   static_cast<ScAddr>(pa3))));
  }

  return bp::object();
}

bp::object _context_iterator5(ScMemoryContext & self,
                              bp::object & param1,
                              bp::object & param2,
                              bp::object & param3,
                              bp::object & param4,
                              bp::object & param5)
{

  // param2 and param4 are always ScType
  bp::extract<ScType> pt2(param2);
  if (!pt2.check())
    return bp::object();

  bp::extract<ScType> pt4(param4);
  if (!pt4.check())
    return bp::object();

  bp::extract<ScAddr> pa1(param1);
  if (pa1.check())
  {
    bp::extract<ScAddr> pa3(param3);
    if (pa3.check())
    {
      bp::extract<ScAddr> pa5(param5);
      if (pa5.check()) // f_a_f_a_f
        return bp::object(PyIterator5(self.Iterator5(static_cast<ScAddr>(pa1),
                                                     static_cast<ScType>(pt2),
                                                     static_cast<ScAddr>(pa3),
                                                     static_cast<ScType>(pt4),
                                                     static_cast<ScAddr>(pa5))));

      bp::extract<ScType> pt5(param5);
      if (pt5.check()) // f_a_f_a_a
        return bp::object(PyIterator5(self.Iterator5(static_cast<ScAddr>(pa1),
                                                     static_cast<ScType>(pt2),
                                                     static_cast<ScAddr>(pa3),
                                                     static_cast<ScType>(pt4),
                                                     static_cast<ScType>(pt5))));
    }

    bp::extract<ScType> pt3(param3);
    if (!pt3.check())
      return bp::object();

    bp::extract<ScAddr> pa5(param5);
    if (pa5.check()) // f_a_a_a_f
      return bp::object(PyIterator5(self.Iterator5(static_cast<ScAddr>(pa1),
                                                   static_cast<ScType>(pt2),
                                                   static_cast<ScType>(pt3),
                                                   static_cast<ScType>(pt4),
                                                   static_cast<ScAddr>(pa5))));

    bp::extract<ScType> pt5(param5);
    if (pt5.check()) // f_a_a_a_a
      return bp::object(PyIterator5(self.Iterator5(static_cast<ScAddr>(pa1),
                                                   static_cast<ScType>(pt2),
                                                   static_cast<ScType>(pt3),
                                                   static_cast<ScType>(pt4),
                                                   static_cast<ScType>(pt5))));
  }

  bp::extract<ScType> pt1(param1);
  if (pt1.check())
  {
    bp::extract<ScAddr> pa3(param3);
    if (!pa3.check())
      return bp::object();

    bp::extract<ScAddr> pa5(param5);
    if (pa5.check()) // a_a_f_a_f
      return bp::object(PyIterator5(self.Iterator5(static_cast<ScType>(pt1),
                                                   static_cast<ScType>(pt2),
                                                   static_cast<ScAddr>(pa3),
                                                   static_cast<ScType>(pt4),
                                                   static_cast<ScAddr>(pa5))));

    bp::extract<ScType> pt5(param5);
    if (pt5.check()) // a_a_f_a_a
      return bp::object(PyIterator5(self.Iterator5(static_cast<ScType>(pt1),
                                                   static_cast<ScType>(pt2),
                                                   static_cast<ScAddr>(pa3),
                                                   static_cast<ScType>(pt4),
                                                   static_cast<ScType>(pt5))));
  }

  return bp::object();
}

bp::object _context_helperResolveSysIdtf(ScMemoryContext & self, bp::object & idtf, bp::object const & type = bp::object())
{
  bp::extract<std::string> se(idtf);
  if (!se.check())
  {
    SC_THROW_EXCEPTION(utils::ExceptionInvalidType,
                       "First parameter should have an instance of str");
  }
  ScType rawType;

  if (!type.is_none())
  {
    bp::extract<ScType> te(type);
    if (!te.check())
    {
      SC_THROW_EXCEPTION(utils::ExceptionInvalidType,
                         "Second parameter should be None or instance of ScType")
    }
    rawType = static_cast<ScType>(te);
  }

  std::string const idtfValue = static_cast<std::string>(se);
  ScAddr resultAddr = self.HelperResolveSystemIdtf(idtfValue, rawType);
  return bp::object(resultAddr);
}

ScTemplateItemValue ResolveTemplateParam(bp::object & p)
{
  bp::extract<impl::PyTemplateItemValue> ve(p);
  if (ve.check())
    return static_cast<impl::PyTemplateItemValue>(ve).GetItemRef();

  bp::extract<std::string> se(p);
  if (se.check())
    return ScTemplateItemValue(static_cast<std::string>(se));

  bp::extract<ScAddr> ae(p);
  if (ae.check())
    return ScTemplateItemValue(static_cast<ScAddr>(ae));

  bp::extract<ScType> te(p);
  if (te.check())
    return ScTemplateItemValue(static_cast<ScType>(te));

  SC_THROW_EXCEPTION(utils::ExceptionInvalidType,
                     "Parameter to template should be ScAddr, ScType, string or replacement");
  return{};
};

class PyTemplate
{
public:
  PyTemplate() : m_impl(new ScTemplate()) {}

  void Triple(bp::object & param1, bp::object & param2, bp::object & param3)
  {
    m_impl->Triple(ResolveTemplateParam(param1),
                   ResolveTemplateParam(param2),
                   ResolveTemplateParam(param3));
  }

  void TripleWithRelation(bp::object & param1, bp::object & param2,
                          bp::object & param3, bp::object & param4, bp::object & param5)
  {
    m_impl->TripleWithRelation(ResolveTemplateParam(param1),
                               ResolveTemplateParam(param2),
                               ResolveTemplateParam(param3),
                               ResolveTemplateParam(param4),
                               ResolveTemplateParam(param5));
  }

  ScTemplate & GetItemRef() const
  {
    return *m_impl;
  }

private:
  std::shared_ptr<ScTemplate> m_impl;
};

class PyTemplateGenParams
{
public:
  PyTemplateGenParams() : m_impl(new ScTemplateGenParams()) {}

  void Add(std::string const & paramName, ScAddr const & value)
  {
    m_impl->Add(paramName, value);
  }

  bp::object Get(std::string const & paramName)
  {
    ScAddr result;
    if (m_impl->Get(paramName, result))
      return bp::object(result);

    return bp::object();
  }

  bool IsEmpty() const
  {
    return m_impl->IsEmpty();
  }

  ScTemplateGenParams & GetItemRef() const
  {
    return *m_impl;
  }

private:
  std::shared_ptr<ScTemplateGenParams> m_impl;
};

bp::object _context_helperGenTemplate(ScMemoryContext & self, PyTemplate & templ, PyTemplateGenParams & params)
{
  PyTemplateGenResult result;
  if (self.HelperGenTemplate(templ.GetItemRef(), result.GetResultRef(), params.GetItemRef()))
    return bp::object(result);

  return bp::object();
}

bp::object _context_helperSearchTemplate(ScMemoryContext & self, PyTemplate & templ)
{
  PyTemplateSearchResult result;
  self.HelperSearchTemplate(templ.GetItemRef(), result.GetResultRef());
  return bp::object(result);
}

bp::object _context_helperBuildTemplate(ScMemoryContext & self, ScAddr const & templAddr)
{
  PyTemplate templ;
  if (self.HelperBuildTemplate(templ.GetItemRef(), templAddr))
    return bp::object(templ);

  return bp::object();
}

boost::shared_ptr<ScMemoryContext> _context_new(std::string const & name)
{
  return boost::shared_ptr<ScMemoryContext>(new ScMemoryContext(sc_access_lvl_make_min, name));
}

bp::object ScAddrFromHash(ScAddr::HashType const value)
{
  return bp::object(ScAddr(value));
}

} // namespace impl

BOOST_PYTHON_MODULE(sc)
{
  bp::register_exception_translator<utils::ScException>(&translateException);
  bp::register_ptr_to_python<boost::shared_ptr<ScMemoryContext>>();

  def("createScMemoryContext", bp::make_function(impl::_context_new, py::ReleaseGILPolicy()));
  def("ScAddrFromHash", bp::make_function(impl::ScAddrFromHash, py::ReleaseGILPolicy()));

  bp::class_<ScMemoryContext, boost::noncopyable>("ScMemoryContext", bp::no_init)
    .def("CreateNode", bp::make_function(&ScMemoryContext::CreateNode, py::ReleaseGILPolicy()))
    .def("CreateEdge", bp::make_function(&ScMemoryContext::CreateEdge, py::ReleaseGILPolicy()))
    .def("CreateLink", bp::make_function(&ScMemoryContext::CreateLink, py::ReleaseGILPolicy()))
    .def("DeleteElement", bp::make_function(&ScMemoryContext::EraseElement, py::ReleaseGILPolicy()))
    .def("GetName", bp::make_function(&ScMemoryContext::GetName, py::ReleaseGILPolicy()))
    .def("IsElement", bp::make_function(&ScMemoryContext::IsElement, py::ReleaseGILPolicy()))
    .def("GetElementType", bp::make_function(&ScMemoryContext::GetElementType, py::ReleaseGILPolicy()))
    .def("GetEdgeInfo", bp::make_function(impl::_context_getEdgeInfo, py::ReleaseGILPolicy()))
    .def("SetLinkContent", bp::make_function(impl::_context_setLinkContent, py::ReleaseGILPolicy()))
    .def("GetLinkContent", bp::make_function(impl::_context_getLinkContent, py::ReleaseGILPolicy()))
    .def("Iterator3", bp::make_function(impl::_context_iterator3, py::ReleaseGILPolicy()))
    .def("Iterator5", bp::make_function(impl::_context_iterator5, py::ReleaseGILPolicy()))
    .def("HelperResolveSystemIdtf", bp::make_function(impl::_context_helperResolveSysIdtf, py::ReleaseGILPolicy()))
    .def("HelperSetSystemIdtf", bp::make_function(&ScMemoryContext::HelperSetSystemIdtf, py::ReleaseGILPolicy()))
    .def("HelperGetSystemIdtf", bp::make_function(&ScMemoryContext::HelperGetSystemIdtf, py::ReleaseGILPolicy()))
    .def("HelperCheckEdge", bp::make_function(&ScMemoryContext::HelperCheckEdge, py::ReleaseGILPolicy()))
    .def("HelperGenTemplate", bp::make_function(impl::_context_helperGenTemplate, py::ReleaseGILPolicy()))
    .def("HelperSearchTemplate", bp::make_function(impl::_context_helperSearchTemplate, py::ReleaseGILPolicy()))
    .def("HelperBuildTemplate", bp::make_function(impl::_context_helperBuildTemplate, py::ReleaseGILPolicy()))
    ;

  bp::class_<impl::PyIterator3>("ScIterator3", bp::no_init)
    .def("Next", bp::make_function(&impl::PyIterator3::Next, py::ReleaseGILPolicy()))
    .def("IsValid", bp::make_function(&impl::PyIterator3::IsValid, py::ReleaseGILPolicy()))
    .def("Get", bp::make_function(&impl::PyIterator3::Get, py::ReleaseGILPolicy()))
    ;

  bp::class_<impl::PyIterator5>("ScIterator5", bp::no_init)
    .def("Next", bp::make_function(&impl::PyIterator5::Next, py::ReleaseGILPolicy()))
    .def("IsValid", bp::make_function(&impl::PyIterator5::IsValid, py::ReleaseGILPolicy()))
    .def("Get", bp::make_function(&impl::PyIterator5::Get, py::ReleaseGILPolicy()))
    ;

  bp::class_<impl::PyLinkContent>("ScLinkContent", bp::no_init)
    .def("AsString", bp::make_function(&impl::PyLinkContent::AsString, py::ReleaseGILPolicy()))
    .def("AsInt", bp::make_function(&impl::PyLinkContent::AsInt, py::ReleaseGILPolicy()))
    .def("AsFloat", bp::make_function(&impl::PyLinkContent::AsDouble, py::ReleaseGILPolicy()))
    ;

  bp::class_<impl::PyTemplateGenResult>("ScTemplateGenResult", bp::no_init)
    .def("Size", bp::make_function(&impl::PyTemplateGenResult::Size, py::ReleaseGILPolicy()))
    .def("__getitem__", bp::make_function(&impl::PyTemplateGenResult::Get, py::ReleaseGILPolicy()))
    ;

  bp::class_<impl::PyTemplateSearchResultItem>("ScTemplateSearchResultItem", bp::no_init)
    .def("Size", bp::make_function(&impl::PyTemplateSearchResultItem::Size, py::ReleaseGILPolicy()))
    .def("__getitem__", bp::make_function(&impl::PyTemplateSearchResultItem::Get, py::ReleaseGILPolicy()))
    ;

  bp::class_<impl::PyTemplateSearchResult>("ScTemplateSearchResult", bp::no_init)
    .def("Size", bp::make_function(&impl::PyTemplateSearchResult::Size, py::ReleaseGILPolicy()))
    .def("__getitem__", bp::make_function(&impl::PyTemplateSearchResult::Get, py::ReleaseGILPolicy()))
    ;

  bp::class_<impl::PyTemplateItemValue>("ScTemplateItemValue", bp::no_init)
    ;

  bp::class_<impl::PyTemplateGenParams>("ScTemplateGenParams", bp::init<>())
    .def("Add", bp::make_function(&impl::PyTemplateGenParams::Add, py::ReleaseGILPolicy()))
    .def("Get", bp::make_function(&impl::PyTemplateGenParams::Get, py::ReleaseGILPolicy()))
    .def("IsEmpty", bp::make_function(&impl::PyTemplateGenParams::IsEmpty, py::ReleaseGILPolicy()))
    ;

  bp::class_<impl::PyTemplate>("ScTemplate", bp::init<>())
    .def("Triple", bp::make_function(&impl::PyTemplate::Triple, py::ReleaseGILPolicy()))
    .def("TripleWithRelation", bp::make_function(&impl::PyTemplate::TripleWithRelation, py::ReleaseGILPolicy()))
    ;

  bp::class_<ScAddr>("ScAddr", bp::init<>())
    .def("IsValid", bp::make_function(&ScAddr::IsValid, py::ReleaseGILPolicy()))
    .def("ToInt", bp::make_function(&ScAddr::Hash, py::ReleaseGILPolicy()))
    .def("__eq__", bp::make_function(&ScAddr::operator==, py::ReleaseGILPolicy()))
    .def("__ne__", bp::make_function(&ScAddr::operator!=, py::ReleaseGILPolicy()))
    .def("__rshift__", bp::make_function(impl::_scAddrToRShift, py::ReleaseGILPolicy()))
    .def("rshift", bp::make_function(impl::_scAddrToRShift, py::ReleaseGILPolicy()))
    ;

  bp::class_<ScType>("ScType", bp::init<>())
    .def(bp::init<ScType::RealType>())
    .def("__eq__", bp::make_function(&ScType::operator==, py::ReleaseGILPolicy()))
    .def("__ne__", bp::make_function(&ScType::operator!=, py::ReleaseGILPolicy()))
    .def("__or__", bp::make_function(&ScType::operator|, py::ReleaseGILPolicy()))
    .def("__and__", bp::make_function(&ScType::operator&, py::ReleaseGILPolicy()))
    .def("__rshift__", bp::make_function(impl::_scTypeToRShift, py::ReleaseGILPolicy()))
    .def("rshift", bp::make_function(impl::_scTypeToRShift, py::ReleaseGILPolicy()))
    .def("IsLink", &ScType::IsLink)
    .def("IsEdge", &ScType::IsEdge)
    .def("IsNode", &ScType::IsNode)
    .def("IsValid", &ScType::IsValid)
    .def("IsConst", &ScType::IsConst)
    .def("IsVar", &ScType::IsVar)
    .def("ToInt", &ScType::operator*)

    .def_readonly("Unknown", &ScType::Unknown)
    .def_readonly("Const", &ScType::Const)
    .def_readonly("Var", &ScType::Var)
    .def_readonly("Node", &ScType::Node)
    .def_readonly("Link", &ScType::Link)

    .def_readonly("EdgeUCommon", &ScType::EdgeUCommon)
    .def_readonly("EdgeDCommon", &ScType::EdgeDCommon)
    .def_readonly("EdgeUCommonConst", &ScType::EdgeUCommonConst)
    .def_readonly("EdgeDCommonConst", &ScType::EdgeDCommonConst)
    .def_readonly("EdgeAccess", &ScType::EdgeAccess)
    .def_readonly("EdgeAccessConstPosPerm", &ScType::EdgeAccessConstPosPerm)
    .def_readonly("EdgeAccessConstNegPerm", &ScType::EdgeAccessConstNegPerm)
    .def_readonly("EdgeAccessConstFuzPerm", &ScType::EdgeAccessConstFuzPerm)
    .def_readonly("EdgeAccessConstPosTemp", &ScType::EdgeAccessConstPosTemp)
    .def_readonly("EdgeAccessConstNegTemp", &ScType::EdgeAccessConstNegTemp)
    .def_readonly("EdgeAccessConstFuzTemp", &ScType::EdgeAccessConstFuzTemp)
    .def_readonly("EdgeUCommonVar", &ScType::EdgeUCommonVar)
    .def_readonly("EdgeDCommonVar", &ScType::EdgeDCommonVar)
    .def_readonly("EdgeAccessVarPosPerm", &ScType::EdgeAccessVarPosPerm)
    .def_readonly("EdgeAccessVarNegPerm", &ScType::EdgeAccessVarNegPerm)
    .def_readonly("EdgeAccessVarFuzPerm", &ScType::EdgeAccessVarFuzPerm)
    .def_readonly("EdgeAccessVarPosTemp", &ScType::EdgeAccessVarPosTemp)
    .def_readonly("EdgeAccessVarNegTemp", &ScType::EdgeAccessVarNegTemp)
    .def_readonly("EdgeAccessVarFuzTemp", &ScType::EdgeAccessVarFuzTemp)

    .def_readonly("NodeConst", &ScType::NodeConst)
    .def_readonly("NodeVar", &ScType::NodeVar)
    .def_readonly("LinkConst", &ScType::LinkConst)
    .def_readonly("LinkVar", &ScType::LinkVar)

    .def_readonly("NodeConstStruct", &ScType::NodeConstStruct)
    .def_readonly("NodeConstTuple", &ScType::NodeConstTuple)
    .def_readonly("NodeConstRole", &ScType::NodeConstRole)
    .def_readonly("NodeConstNoRole", &ScType::NodeConstNoRole)
    .def_readonly("NodeConstClass", &ScType::NodeConstClass)
    .def_readonly("NodeConstAbstract", &ScType::NodeConstAbstract)
    .def_readonly("NodeConstMaterial", &ScType::NodeConstMaterial)

    .def_readonly("NodeVarStruct", &ScType::NodeVarStruct)
    .def_readonly("NodeVarTuple", &ScType::NodeVarTuple)
    .def_readonly("NodeVarRole", &ScType::NodeVarRole)
    .def_readonly("NodeVarNoRole", &ScType::NodeVarNoRole)
    .def_readonly("NodeVarClass", &ScType::NodeVarClass)
    .def_readonly("NodeVarAbstract", &ScType::NodeVarAbstract)
    .def_readonly("NodeVarMaterial", &ScType::NodeVarMaterial)
    ;
}


} // namespace 

namespace py
{

void ScPythonMemoryModule::Initialize()
{
  PyImport_AppendInittab("sc", &PyInit_sc);
}

} // namespace py

