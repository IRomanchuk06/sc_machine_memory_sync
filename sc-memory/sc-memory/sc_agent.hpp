/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#pragma once

#include "sc-memory/sc_object.hpp"
#include "sc-memory/sc_keynodes.hpp"

#include "sc_agent_context.hpp"

#include "sc-memory/utils/sc_log.hpp"

#define SC_AGENT_LOG_DEBUG(__msg__) SC_LOG_DEBUG(GetName() << " " << __msg__)
#define SC_AGENT_LOG_INFO(__msg__) SC_LOG_INFO(GetName() << " " << __msg__)
#define SC_AGENT_LOG_WARNING(__msg__) SC_LOG_WARNING(GetName() << " " << __msg__)
#define SC_AGENT_LOG_ERROR(__msg__) SC_LOG_ERROR(GetName() << " " << __msg__)

/// Defines `std::string GetName()` method for agent class. It is used in sc-memory logging.
#define SC_AGENT_BODY(__AgentName__) \
public: \
  _SC_EXTERN explicit __AgentName__(ScAddr const & userAddr = ScAddr::Empty) \
    : ScAgent(userAddr) \
  { \
  } \
  static _SC_EXTERN std::string GetName() \
  { \
    return #__AgentName__; \
  }

/// Defines `std::string GetName()` method for agent class. It is used in sc-memory logging.
#define SC_ACTION_AGENT_BODY(__AgentName__) \
public: \
  _SC_EXTERN explicit __AgentName__(ScAddr const & userAddr = ScAddr::Empty) \
    : ScActionAgent(userAddr) \
  { \
  } \
  static _SC_EXTERN std::string GetName() \
  { \
    return #__AgentName__; \
  }

/*!
 * @interface An interface for agents classes
 * @note This class is an API to implement your own registration API of agents.
 */
class _SC_EXTERN ScAgentAbstract : public ScObject
{
public:
  _SC_EXTERN ~ScAgentAbstract() override;

  /*! @details It will be called, if the event the agent is subscribed to has been initialized. In this method,
   * the logic implemented by the agent starts running. Depending on the event type, the number of valid arguments to
   * this method changes.
   * @param[in] listenAddr An address of a subscripting element (can be empty address)
   * @param[in] edgeAddr An address of an edge element that come in or from subscripting element (can be empty address)
   * @param[in] otherAddr An address of the element incident to the edge (can be empty address)
   * @return sc_result A status code of agent on it finish
   */
  _SC_EXTERN virtual sc_result OnEvent(
      ScAddr const & listenAddr,
      ScAddr const & edgeAddr,
      ScAddr const & otherAddr) = 0;

  /*! Registers agent instance on subscripting addresses
   * @param addrs A vector of subscripting addresses
   * @return A pointer to agent instance
   */
  static _SC_EXTERN void Register(ScMemoryContext * ctx, ScAddrVector const & addrs) {};

  /*! Registers agent instance on subscripting address
   * @param addr A subscripting address
   * @return A pointer to agent instance
   */
  static _SC_EXTERN void Register(ScMemoryContext * ctx, ScAddr const & addr) {};

  /*! Unregisters agent instance on all subscripting address
   * @return A pointer to agent instance
   */
  static _SC_EXTERN void Unregister(ScMemoryContext * ctx) {};

protected:
  mutable ScAgentContext m_memoryCtx;

  static std::list<ScEvent *> m_events;

  _SC_EXTERN explicit ScAgentAbstract(ScAddr const & userAddr);

  static _SC_EXTERN std::string GetName();

  static _SC_EXTERN ScEvent::DelegateFuncWithUserAddr GetCallback();

private:
  _SC_EXTERN sc_result Initialize(ScMemoryContext * ctx, ScAddr const & initMemoryGeneratedStructureAddr) override;

  _SC_EXTERN sc_result Shutdown(ScMemoryContext * ctx) override;
};

/*!
 * @interface An interface for implementing any agents classes.
 * @note This class is an API to implement your own registration API of agents.
 */
template <ScEvent::Type const & eventType>
class _SC_EXTERN ScAgent : public ScAgentAbstract
{
public:
  template <class AgentClass, class... Args>
  static _SC_EXTERN void Register(ScMemoryContext * ctx, Args const &... addrs)
  {
    static_assert(std::is_abstract<AgentClass>::value == false, "AgentClass must override all virtual functions");

    SC_LOG_INFO("Register " << AgentClass::GetName());

    ScAddrVector const & addrVector{addrs...};
    for (auto const & addr : addrVector)
      m_events.push_back(new ScEvent(*ctx, addr, eventType, AgentClass::template GetCallback<AgentClass>()));
  }

  template <class AgentClass>
  static _SC_EXTERN void Unregister(ScMemoryContext * ctx)
  {
    SC_UNUSED(ctx);

    SC_LOG_INFO("Unregister " << AgentClass::GetName());

    for (auto * event : m_events)
      delete event;

    m_events.clear();
  }

  /*! Complete defined logic if agent is finished successfully (SC_RESULT_OK).
   * @param listenAddr An address of a subscripting element.
   * @param edgeAddr An address of an edge element that come in or from subscripting element.
   * @param otherAddr An address of the element incident to the edge.
   * @return A pointer to agent instance.
   */
  _SC_EXTERN virtual void OnSuccess(ScAddr const & addr, ScAddr const & edgeAddr, ScAddr const & otherAddr) {}

  /*! Complete defined logic if agent is finished unsuccessfully (SC_RESULT_NO).
   * @param listenAddr An address of a subscripting element.
   * @param edgeAddr An address of an edge element that come in or from subscripting element.
   * @param otherAddr An address of the element incident to the edge.
   * @return A pointer to agent instance.
   */
  _SC_EXTERN virtual void OnUnsuccess(ScAddr const & addr, ScAddr const & edgeAddr, ScAddr const & otherAddr) {}

  /*! Complete defined logic if agent is finished with error.
   * @param listenAddr An addr of a subscripting element.
   * @param edgeAddr An addr of an edge element that come in or from subscripting element.
   * @param otherAddr An address of the element incident to the edge.
   * @param errorCode An error code of agent.
   * @return A pointer to agent instance.
   */
  _SC_EXTERN virtual void OnError(
      ScAddr const & addr,
      ScAddr const & edgeAddr,
      ScAddr const & otherAddr,
      sc_result errorCode)
  {
  }

protected:
  _SC_EXTERN explicit ScAgent(ScAddr const & userAddr)
    : ScAgentAbstract(userAddr){};

  template <class AgentClass>
  static _SC_EXTERN ScEvent::DelegateFuncWithUserAddr GetCallback()
  {
    return [](ScAddr const & userAddr,
              ScAddr const & addr,
              ScAddr const & edgeAddr,
              ScType const & edgeType,
              ScAddr const & otherAddr) -> sc_result
    {
      AgentClass agent(userAddr);
      SC_LOG_INFO(AgentClass::GetName() << " started");
      sc_result const result = agent.OnEvent(addr, edgeAddr, otherAddr);

      // finish agent
      if (result == SC_RESULT_OK)
      {
        SC_LOG_INFO(AgentClass::GetName() << " finished successfully");
        agent.OnSuccess(addr, edgeAddr, otherAddr);
      }
      else if (result == SC_RESULT_NO)
      {
        SC_LOG_INFO(AgentClass::GetName() << " finished unsuccessfully");
        agent.OnUnsuccess(addr, edgeAddr, otherAddr);
      }
      else
      {
        SC_LOG_INFO(AgentClass::GetName() << " finished with error");
        agent.OnError(addr, edgeAddr, otherAddr, result);
      }

      return result;
    };
  }
};

/*!
 * @class This class is an API to implement your own agent classes.
 * @details The `sc_result OnEvent(ScAddr const & actionAddr)` procedure should be implemented in the child class.
 * You can also override methods `void OnSuccess(ScAddr const & actionAddr)`, `void OnUnsuccess(ScAddr const &
 * actionAddr)` and `void OnError(ScAddr const & actionAddr, sc_result errorCode)`.
 * They are executed depending on the agent error code returned in the function `OnEvent`.
 *
 * File sc_syntactic_analysis_agent.hpp:
 * \code
 * #include <sc-memory/sc_agent.hpp>
 *
 * #include "nlp-module/keynodes/sc_nlp_keynodes.hpp"
 *
 * #include "nlp-module/analyser/sc_syntactic_analyser.hpp"
 *
 * namespace nlp
 * {
 *
 * * @details The `ScSyntacticAnalysisAgent` class inherits from `ScAgent` and overrides the `OnEvent` and `OnSuccess`
 * * methods.
 * * It uses the `SC_AGENT_BODY` macro and has a member `m_analyser` of type `nlp::ScSyntacticAnalyser`.
 * * The main logic is implemented in the `OnEvent` method.
 * class ScSyntacticAnalysisAgent : public ScActionAgent<ScKeynodes::kSyntacticAnalyseAction>
 * {
 * public:
 *   SC_ACTION_AGENT_BODY(ScSyntacticAnalysisAgent);
 *
 *   ScSyntacticAnalysisAgent();
 *
 *   sc_result OnEvent(ScAddr const & actionAddr) override;
 *
 *   void OnSuccess(ScAddr const & actionAddr) override;
 * };
 *
 * private:
 *   nlp::ScSyntacticAnalyser * m_analyser;
 * }
 * \endcode
 *
 * File sc_syntactic_analysis_agent.cpp:
 * \code
 * #include "nlp-module/agents/sc_syntactic_analysis_agent.hpp"
 *
 * ScSyntacticAnalysisAgent::ScSyntacticAnalysisAgent()
 *   : m_analyser(new nlp::ScSyntacticAnalyser(m_memoryCtx))
 * {
 * }
 *
 * sc_result ScSyntacticAnalysisAgent::OnEvent(ScAddr const & actionAddr) override
 * {
 *   ScAddr const textAddr = m_memoryCtx.GetArgument(actionAddr, 1);
 *   if (!textAddr.IsValid())
 *   {
 *     SC_AGENT_LOG_ERROR("Invalid text link addr");
 *     return SC_RESULT_ERROR_INVALID_PARAMS;
 *   }
 *
 *   try
 *   {
 *     ScAddr const lexemeTreeAddr = m_analyser->Analyse(textAddr);
 *   }
 *   catch(utils::ScException const & e)
 *   {
 *     SC_AGENT_LOG_ERROR("Error occurred: " << e.Message());
 *     return SC_RESULT_ERROR_INVALID_STATE;
 *   }
 *
 *   if (!lexemeTreeAddr.IsValid())
 *   {
 *     SC_AGENT_LOG_INFO("Lexeme tree hasn't been formed");
 *     return SC_RESULT_NO;
 *   }
 *
 *   SC_AGENT_LOG_INFO("Lexeme tree has been formed");
 *   ScAddr const & answerAddr = m_memoryCtx->FormStructure({lexemeTreeAddr});
 *   m_memoryCtx.FormAnswer(actionAddr, answerAddr);
 *   return SC_RESULT_OK;
 * }
 *
 * void ScSyntacticAnalysisAgent::OnSuccess(ScAddr const & actionAddr) override
 * {
 *   delete m_memoryCtx->InitializeEvent<ScEvent::Type::AddOutputEdge>(
 *   nlp::ScNLPKeynodes::kSyntacticSynthesizeAction,
 *     [this]() -> {
 *       ScAddr const & addr = m_memoryCtx.CreateNode(ScType::NodeConst);
 *       m_memoryCtx.CreateEdge(
 *         ScType::EdgeAccessConstPosPerm, nlp::ScNLPKeynodes::kSyntacticSynthesizeAction, addr);
 *     },
 *     [this](ScAddr const & listenAddr, ScAddr const & edgeAddr, ScAddr const & otherAddr) -> sc_result {
 *       return m_memoryCtx.HelperCheckEdge(nlp::ScNLPKeynodes::kQuestionFinishedSucessfully, edgeAddr,
 ScType::EdgeAccessConstPosPerm)
 *         ? SC_RESULT_OK
 *         : SC_RESULT_NO;
 *     }
 *   )->Wait(10000);
 * }
 * \endcode
 */
template <ScKeynodeClass const & actionClass = ScKeynodes::kEmptyClass>
class _SC_EXTERN ScActionAgent : public ScAgent<ScEvent::Type::AddOutputEdge>
{
public:
  _SC_EXTERN explicit ScActionAgent(ScAddr const & userAddr)
    : ScAgent(userAddr)
  {
  }

  /*! @details This method is called when the event the agent is subscribed to is initialized.
   * It implements the main logic of the agent.
   * @param[in] actionAddr An address of the action sc-element.
   * @return sc_result A status code indicating the result of the agent's execution.
   */
  _SC_EXTERN virtual sc_result OnEvent(ScAddr const & actionAddr) = 0;

  /*! @brief Completes defined logic if the agent finishes successfully (SC_RESULT_OK).
   * @param actionAddr An address of the action sc-element.
   */
  _SC_EXTERN virtual void OnSuccess(ScAddr const & actionAddr) {}

  /*! @brief Completes defined logic if the agent finishes unsuccessfully (SC_RESULT_NO).
   * @param actionAddr An address of the action sc-element.
   */
  _SC_EXTERN virtual void OnUnsuccess(ScAddr const & actionAddr) {}

  /*! @brief Completes defined logic if the agent finishes with an error.
   * @param actionAddr An address of the action element.
   * @param errorCode An error code indicating the type of error.
   */
  _SC_EXTERN virtual void OnError(ScAddr const & actionAddr, sc_result errorCode) {}

  /*!
   * @brief Provides a callback function for handling events in the sc-memory.
   *
   * @tparam AgentClass The class of the agent that will handle the event.
   * @return ScEvent::DelegateFuncWithUserAddr A delegate function that processes the event.
   *
   * @details This function template returns a lambda function that serves as a callback for event handling.
   *          The callback function:
   *          - Checks if initiated action has specified action class for this agent.
   *          - Creates an instance of the specified agent class.
   *          - Calls the `OnEvent` method of the agent to handle the event.
   *          - Depending on the result of the `OnEvent` method, it calls `OnSuccess`, `OnUnsuccess`, or `OnError`
   * methods of the agent.
   *          - Logs the status of the agent's execution and creates corresponding edges in the sc-memory.
   */
  template <class AgentClass>
  static _SC_EXTERN ScEvent::DelegateFuncWithUserAddr GetCallback()
  {
    return [](ScAddr const & userAddr,
              ScAddr const & addr,
              ScAddr const & edgeAddr,
              ScType const & edgeType,
              ScAddr const & otherAddr) -> sc_result
    {
      sc_result result = SC_RESULT_ERROR;
      if (!(edgeType.BitAnd(ScType::EdgeAccess) == ScType::EdgeAccess)
          || (actionClass.IsValid()
              && !ScMemory::ms_globalContext->HelperCheckEdge(actionClass, otherAddr, ScType::EdgeAccessConstPosPerm)))
        return result;

      AgentClass agent(userAddr);
      SC_LOG_INFO(AgentClass::GetName() << " started");
      result = agent.OnEvent(otherAddr);

      // finish agent
      if (result == SC_RESULT_OK)
      {
        SC_LOG_INFO(AgentClass::GetName() << " finished successfully");
        agent.OnSuccess(otherAddr);
        ScMemory::ms_globalContext->CreateEdge(
            ScType::EdgeAccessConstPosPerm, ScKeynodes::kQuestionFinishedSuccessfully, otherAddr);
      }
      else if (result == SC_RESULT_NO)
      {
        SC_LOG_INFO(AgentClass::GetName() << " finished unsuccessfully");
        agent.OnUnsuccess(otherAddr);
        ScMemory::ms_globalContext->CreateEdge(
            ScType::EdgeAccessConstPosPerm, ScKeynodes::kQuestionFinishedUnsuccessfully, otherAddr);
      }
      else
      {
        SC_LOG_INFO(AgentClass::GetName() << " finished with error");
        agent.OnError(otherAddr, result);
        ScMemory::ms_globalContext->CreateEdge(
            ScType::EdgeAccessConstPosPerm, ScKeynodes::kQuestionFinishedWithError, otherAddr);
      }
      ScMemory::ms_globalContext->CreateEdge(ScType::EdgeAccessConstPosPerm, ScKeynodes::kQuestionFinished, otherAddr);

      return result;
    };
  }

private:
  _SC_EXTERN sc_result OnEvent(ScAddr const & listenAddr, ScAddr const & edgeAddr, ScAddr const & otherAddr) final
  {
    return SC_RESULT_OK;
  }

  _SC_EXTERN void OnSuccess(ScAddr const & addr, ScAddr const & edgeAddr, ScAddr const & otherAddr) final {}

  _SC_EXTERN void OnUnsuccess(ScAddr const & addr, ScAddr const & edgeAddr, ScAddr const & otherAddr) final {}

  _SC_EXTERN void OnError(ScAddr const & addr, ScAddr const & edgeAddr, ScAddr const & otherAddr, sc_result errorCode)
      final
  {
  }
};

/*!
 * @brief Registers an agent in the sc-memory.
 *
 * @param __Context__ The context in which the agent is being registered.
 * @param __AgentName__ The name of the agent class being registered.
 * @param ... List of subscription addresses that can be passed to the Register function.
 *
 * @details This macro expands to call the static Register function of the agent class,
 *          passing the context and any additional arguments provided.
 *          It uses variadic arguments (...) to allow flexible parameter passing.
 */
#define SC_AGENT_REGISTER(__Context__, __AgentName__, ...) \
  __AgentName__::Register<__AgentName__>(__Context__, __VA_ARGS__)

/*!
 * @brief Unregisters an agent in the sc-memory.
 *
 * @param __Context__ The context in which the agent is being unregistered.
 * @param __AgentName__ The name of the agent class being unregistered.
 *
 * @details This macro expands to call the static Unregister function of the agent class,
 *          passing the context.
 */
#define SC_AGENT_UNREGISTER(__Context__, __AgentName__) __AgentName__::Unregister<__AgentName__>(__Context__)

/*!
 * @brief Defines the implementation of the OnEvent method for an agent class.
 *
 * @param __AgentName__ The name of the agent class.
 *
 * @details This macro expands to define the OnEvent method of the specified agent class,
 *          which handles events in the sc-memory.
 */
#define SC_AGENT_IMPLEMENTATION(__AgentName__) \
  sc_result __AgentName__::OnEvent(ScAddr const & listenAddr, ScAddr const & edgeAddr, ScAddr const & otherAddr)

/*!
 * @brief Defines the implementation of the OnEvent method for an action agent class.
 *
 * @param __AgentName__ The name of the action agent class.
 *
 * @details This macro expands to define the OnEvent method of the specified action agent class,
 *          which handles action events in the sc-memory.
 */
#define SC_ACTION_AGENT_IMPLEMENTATION(__AgentName__) sc_result __AgentName__::OnEvent(ScAddr const & actionAddr)
