// **********************************************************************
//             File : FSCSOAPCommandSession.cc
//        Classname : FSCSOAPCommandSession
//        Author(s) : rdctahu
//          Created : 2002-04-10
//
//
// Copyright (c) 1998 Ericsson Software Technology AB, Sweden.
// All rights reserved.
// The Copyright to the computer program(s) herein is the property of
// Ericsson Software Technology AB, Sweden.
// The program(s) may be used and/or copied with the written permission
// from Ericsson Software Technology AB or in accordance with the terms
// and conditions stipulated in the agreement/contract under which the
// program(s) have been supplied.
//
// **********************************************************************
//
//#pragma ident "$Id: FSCSOAPCommandSession.cc,v 1.5 2000/09/21 18:18:38 epkboli Exp $"
//

// Class header
#include "FSCSOAPCommandSession.hh"

// Standard includes
#ifndef DEPEND
#include <stdlib.h>
#include <rw/ctoken.h>
#include <rw/thr/mutex.h>
#include <rw/sync/RWMutexLock.h>

#endif

// Includes
//#include "FSCSOAPLink.hh"
#include "debug/DebugMacros.hh"
#include "global/FDSGlobalDefs.hh"
#include "namedvalue/FDSNamedValue.hh"
#include "namedvalue/FDSNamedValueBuilder.hh"
#include "exception/FDSException.hh"
#include "FSCSOAPRPCDefs.hh"
#include "generic/FDSOrbix.hh"

using namespace LpSoapRpc;

#define SOAPJAVARPC_CLASSNAME "se.ericsson.ema.downstream.soapjavarpc.SoapJavaRpcImpl"
#define PROTOCOL_PROPERTY_CLASSPATH "ClassPath"
#define PROTOCOL_PROPERTY_CLASSNAME "ClassName"
#define PROTOCOL_PROPERTY_PARAMETERS_DELIMITER "ParaDelimiter"
#define PROTOCOL_PROPERTY_PARAMETERS "Parameters"
#define SOAPRPCLINK_S_IOR_FILENAME "JAVALINKHOST.ior"
#ifdef    SOAPRPC_TEST
#define CLASS_PATH    \
""
#else
#define    CLASS_PATH    ""
#endif

RWCString namespaces[] =
{  // {"ns-prefix", "ns-name"}
   "http://schemas.xmlsoap.org/soap/envelope/",
   "http://schemas.xmlsoap.org/soap/encoding/",
   "http://www.w3.org/1999/XMLSchema-instance",
   "http://www.w3.org/1999/XMLSchema",
   "http://schemas.ericsson.com/cai3g1.0/2003/05/30/",   //bind "ns" namespace prefix
   "http://schemas.ericsson.com/cai3g1.0/2004/01/22/", // bind "ns2" namespace prefix
   "http://schemas.ericsson.com/cai3g1.1/", // bind "ns3" namespace prefix
   "http://schemas.ericsson.com/cai3g1.2/", // bind "ns4" namespace prefix
   ""
};



// **********************************************************************
//    INITIALIZE
// **********************************************************************

void initialize()
{
    FDS_DEBUG("[SoapRpcLNK] create the MasterInstance, and register to LinkManager session Map ");
    FSCSOAPCommandSession* masterInstance = new FSCSOAPCommandSession();
}


// **********************************************************************
//    CONSTRUCTOR
// **********************************************************************

FSCSOAPCommandSession::FSCSOAPCommandSession(const RWCString& neName, const FDSNamedValue *protocolParams)
        : FSCNeCmdSession (neName)
{

      // Get the protocol parameters from the configuration named-value.
      //
    const FDSNamedValue *urlNV;
    const FDSNamedValue *urlpNV;
    const FDSNamedValue *urlsNV;
    const FDSNamedValue *urlnNV;
    const FDSNamedValue *urnNV;
    const FDSNamedValue *namespaceNV;
    const FDSNamedValue *userNV;
    const FDSNamedValue *pwdNV;
    const FDSNamedValue *respTimeoutNV;
    const FDSNamedValue *carryLogIdNV;
    RWCString m_urlp,m_urls,m_urln,m_usr,m_pwd,m_respTimeout,m_carryLogId;
    FDS_DEBUG("********protocolParams");
    std::cout<<"debug soaprpc";
    FDS_DEBUG(*protocolParams);

    urlNV    = protocolParams->itemWithName("URL");
    if (urlNV == NULL)
    {
        urlpNV    = protocolParams->itemWithName("URLProvisioning");
        urlsNV    = protocolParams->itemWithName("URLSessionControl");
        urlnNV    = protocolParams->itemWithName("URLNotification");
    }
    else
    {
        urlpNV     = urlNV;
        urlsNV     = urlNV;
        urlnNV     = urlNV;
    }

    urnNV    = protocolParams->itemWithName("URN");
    namespaceNV = protocolParams->itemWithName("NameSpace");
    userNV   = protocolParams->itemWithName("User");
    pwdNV    = protocolParams->itemWithName("Password");
    respTimeoutNV    = protocolParams->itemWithName("RespTimeout");
    carryLogIdNV     = protocolParams->itemWithName("CarryLogId");


    if ( respTimeoutNV == NULL )
    {
        //In order to handle service network, we need be able to read the parameter with
        //a name directly encoded from ASN.1
        //that is, without the translation in SystemMgmt (NE::create.pc)
        respTimeoutNV    = protocolParams->itemWithName("resptimeout");
    }
    if ( respTimeoutNV == NULL )
    {
        m_respTimeout = "0";
    }
    else
    {
        RWCString strTmp      = respTimeoutNV->value();
        if(strTmp.isNull())
            m_respTimeout = "0";
        else
            m_respTimeout = strTmp;
    }

      // Checking if all parameters are included in the configuration
      //
    if ( urlpNV == NULL )
    {
        urlpNV    = protocolParams->itemWithName("urlprovisioning");
    }
    if ( urlpNV == NULL )
    {
        throw FDSException::FDSStandardException("SOAPJAVARPC: Missing URLProvisioning in configuration****", 0);
    }

    m_urlp = urlpNV->value();
    if(m_urlp.isNull())
        throw FDSException::FDSStandardException("SOAPJAVARPC: Missing URLProvisioning in configuration*", 0);

    if ( urlsNV == NULL )
    {
        urlsNV    = protocolParams->itemWithName("urlsessioncontrol");
    }
    if ( urlsNV == NULL )
    {
        throw FDSException::FDSStandardException("SOAPJAVARPC: Missing URLSessionControl in configuration******", 0);
    }

    m_urls = urlsNV->value();
    if(m_urls.isNull())
        throw FDSException::FDSStandardException("SOAPJAVARPC: Missing URLSessionControl in configuration**", 0);

    if ( urlnNV == NULL )
    {
        urlnNV    = protocolParams->itemWithName("urlnotification");
    }
    if ( urlnNV == NULL )
    {
        throw FDSException::FDSStandardException("SOAPJAVARPC: Missing URLNotification in configuration", 0);
    }

    m_urln = urlnNV->value();
    if(m_urln.isNull())
        throw FDSException::FDSStandardException("SOAPJAVARPC: Missing URLNotification in configuration", 0);

    if ( namespaceNV == NULL )
    {
        namespaceNV    = protocolParams->itemWithName("nameSpace");
    }
    if ( namespaceNV == NULL )
    {
        throw FDSException::FDSStandardException("SOAPJAVARPC: Missing NAMEPACE in configuration", 0);
    }

    m_namespace        = namespaceNV->value();
//comment out the two lines of codes to support namespace for UPG
//    if(m_namespace.isNull())
//        throw FDSException::FDSStandardException("SOAPJAVARPC: Missing NAMESPACE in configuration", 0);

    if ( !m_namespace.isNull() )
    {
        if (!isValidityNamespace(m_namespace)){
            throw FDSException::FDSStandardException("SOAPJAVARPC: NAMEPACE config error in configuration", 0);
        }
    }


    if(pwdNV == NULL)
    {
        pwdNV    = protocolParams->itemWithName("password");
    }
    if(pwdNV != NULL)
    {
        m_pwd         = pwdNV->value();
    }
    else
    {
        m_pwd         = "";
    }


    if(userNV == NULL)
    {
        userNV    = protocolParams->itemWithName("user");
    }
    if(userNV != NULL)
    {
        m_usr         = userNV->value();
    }
    else
    {
        m_usr         = "";
    }

    if(carryLogIdNV == NULL)
    {
        carryLogIdNV    = protocolParams->itemWithName("carryLogId");
    }
    if(carryLogIdNV != NULL)
    {
        m_carryLogId = carryLogIdNV->value();
    }
    else
    {
        m_carryLogId = "0";
    }

    FDS_DEBUG("***************m_urlp");
    FDS_DEBUG(m_urlp);
    FDS_DEBUG("***************m_urls");
    FDS_DEBUG(m_urls);
    FDS_DEBUG("***************m_urln");
    FDS_DEBUG(m_urln);
    RWCString parameters = m_urlp + "#" + m_urls + "#" + m_urln + + "#" +m_namespace +"#" + m_usr + "#" + m_pwd \
        + "#" + m_respTimeout + "#" + m_carryLogId;
    //construct the configNV 
    auto_ptr<FDSNamedValue>  a_protocolParams ( new FDSNamedValue());
    RWCString theComponentPath;

    /**
     *     modify the classpath
     */
    theComponentPath = ::getenv(FDSGlobalDefs::FDS_CONTAINER_HOME) + RWCString("/lib");
    FDS_DEBUG("[SOAPJAVARPC] FDS_CONTAINER_HOME = " + theComponentPath);
    RWCString l_classPath = theComponentPath +"/soaprpc.jar";
    //append ClassPath and mainClass
    a_protocolParams->append(new FDSNamedValue(PROTOCOL_PROPERTY_CLASSPATH,l_classPath));
    a_protocolParams->append(new FDSNamedValue(PROTOCOL_PROPERTY_CLASSNAME,SOAPJAVARPC_CLASSNAME));
    a_protocolParams->append(new FDSNamedValue(PROTOCOL_PROPERTY_PARAMETERS_DELIMITER,"#"));
    a_protocolParams->append(new FDSNamedValue(PROTOCOL_PROPERTY_PARAMETERS,parameters));
    initLink((const FDSNamedValue*)a_protocolParams.get());
}

// **********************************************************************
//    DESTRUCTOR
// **********************************************************************

FSCSOAPCommandSession::~FSCSOAPCommandSession()
{
    try {
        disconnect();
        if( m_javaLinkHostPtr!= NULL){
            delete m_javaLinkHostPtr;
        }
    }
    catch ( ... ) {
        cerr << "[FSCSOAPCommandSession::~FSCSOAPCommandSession]  : unkown error when disconnect from [" +
                   getNeName() + "]. Ignore the exception!" << endl;
        if( m_javaLinkHostPtr!= NULL){
            delete m_javaLinkHostPtr;
        }
    }
}

// **********************************************************************
//    CONSTRUCTOR FOR THE MASTER INSTANCE
// **********************************************************************
FSCSOAPCommandSession::FSCSOAPCommandSession(const RWBoolean& a_addProtocolFlag)
            : FSCNeCmdSession(""),m_linkID(""),m_javaLinkHostPtr(NULL)
{
    RWCString strTmp = "SOAPRPC";

    if (a_addProtocolFlag == TRUE)
    {
        addProtocolImplementation(this, strTmp);
    }
}


// **********************************************************************
//    NEW INSTANCE
// **********************************************************************

FSCNeCmdSession* FSCSOAPCommandSession::newInstance(const RWCString& neName, const FDSNamedValue *protocolParams)
{
    FDS_DEBUG("[SOAPRPCLINK] Creating link session");
    FSCSOAPCommandSession* l_session = new FSCSOAPCommandSession(neName, protocolParams);
    return l_session;
}

// **********************************************************************
//    INIT LINK
// **********************************************************************
void FSCSOAPCommandSession::initLink(const FDSNamedValue* a_protocolParams) 
{
    FDS_DEBUG("[SOAPJAVARPC] Initialze the SoapRpc(java) Link");
    //connect to the JavaLinkHost, throw exception if failed to connect to it
    try{
        attachJavaLinkHost();
        //call createJavaLink to remote standalone java application
        m_linkID = m_javaLinkHostPtr->createLink(FDSNV_PTR_IN(a_protocolParams));
        FDS_DEBUG("[SOAPRPCLINK] Set connection");
        m_javaLinkHostPtr->connect(m_linkID);
    }catch (FDSException::FDSStandardException &exp){
        throw exp;
    }catch (CORBA::SystemException &e){
        FDS_ERROR("LpSoapRpcLink::initLink : catch CORBA exception " << e);
        throw new FDSException::FDSStandardException ("Corba Exception accur while call disconnect on javalinkhost ",0);
    }catch(...){
        throw FDSException::FDSStandardException("internal unknown exception error",0);
    }
}

bool FSCSOAPCommandSession::isValidityNamespace(RWCString v_ns)
{
    bool ret = false;
    int i = FSCSOAPRPCDefs::URNPOSITION;
    while (namespaces[i] != "")
    {
        if (v_ns.compareTo(namespaces[i], RWCString::exact) == 0)
        {
            ret = true;
            break;
        }
        i++;
    }
    if (ret)
    {
        FDS_DEBUG("[SOAPJAVARPC]isValidity Namespace is true");
    }
    else
        FDS_DEBUG("[SOAPJAVARPC]isValidity Namespace is false");
    return ret;
}


// **********************************************************************
//    EXECUTE COMMAND
// **********************************************************************

RWCString FSCSOAPCommandSession::executeCommand(const RWCString& a_command)
{
    FDS_DEBUG("[SOAPJAVARPC] get new command : " + a_command);
    FDSNamedValueBuilder builder;
    FDSNamedValue *pNV = builder.build(a_command);
    FDSNamedValue *pNVnamespace = pNV->itemWithName("cai3gNamespace");
    if ( !m_namespace.isNull() ) //if true, must valid
    {
        FDS_DEBUG("[SOAPJAVARPC] Have Original namespace:"+ m_namespace);
        if ( pNVnamespace )
        {
            FDS_DEBUG("[SOAPJAVARPC] remove the namespace transferred by upper level : " + RWCString(pNVnamespace->value()) );
            FDSNamedValue* tmp = pNV->remove(pNVnamespace);
            if(tmp)
            {
                delete tmp;
                tmp = NULL;
            }
        }
    }else {
        if ( pNVnamespace && !RWCString(pNVnamespace->value()).isNull() && isValidityNamespace(pNVnamespace->value()) )
        {
        }else
        {
            FDS_ERROR("GWF:error handling");
            FDS_DEBUG("Catch error when do namespace process");
            throw  FDSException::FDSStandardException("SOAPJAVARPC: Missing NAMESPACE or WRONG in NAMESPACE configuration", 0);
        }
    }
    FDSNamedValue *pNVOper = pNV->itemWithName("operation");
    if ( pNVOper && pNVOper->value() == RWCString("Search") )//for search, delete the data/value tag.
    {
        FDSNamedValue *pNVfilters = pNV->itemWithName("filters");
        if ( pNVfilters )
        {
            FDSNamedValue *pNVfilter = pNVfilters->itemWithName("filter");
            if ( pNVfilter )
            {
                FDSNamedValue *pNVmoAttributes = pNVfilter->itemWithName("moAttributes");
                if ( pNVmoAttributes )
                {
                    FDSNamedValue *removeNode = pNVmoAttributes->itemWithName("data");
                    FDSNamedValue *pNVValue = NULL;
                    if ( removeNode )
                    {
                        pNVValue = pNVmoAttributes->itemWithName("data")->itemWithName("value");
                    }
                    RWCString strValue;
                    if ( pNVValue )
                    {
                        strValue = pNVValue->value();
                    }
                    FDSNamedValue *tmp = removeNode->remove(pNVValue);
                    if(tmp)
                    {
                        delete tmp;
                        tmp = NULL;
                    }
                    pNVmoAttributes->value(strValue);
                }
            }
        }
    }
    RWCString strNewCommand = pNV->toString();
    delete pNV;
    ///now send the command to the java implemeted
    try{
        CORBA::String_var  resp = m_javaLinkHostPtr->sendMessage(m_linkID, strNewCommand);
        return RWCString(resp);
    }catch (FDSException::FDSStandardException &exp){
        //exp.errorCode(0);
        throw exp;
    }catch (CORBA::SystemException &e){
        FDS_ERROR("LpSoapRpcLink::sendMessage : catch CORBA exception " << e);
        throw new FDSException::FDSStandardException ("Corba Exception accur while call disconnect on javalinkhost ",0);
    }catch(...){
        throw FDSException::FDSStandardException("internal unknown exception error",0);
    }
}

// **********************************************************************
//
// **********************************************************************
void FSCSOAPCommandSession::disconnect()
{
    FDS_DEBUG("[JavaLink] ==============================");
    FDS_DEBUG("[JavaLink] disconnecting");
    FDS_DEBUG("[JavaLink] ==============================");
    try{
        m_javaLinkHostPtr->disconnect(m_linkID);
        m_javaLinkHostPtr->destroyLink(m_linkID);
    }catch (FDSException::FDSStandardException &exp){
        //exp.errorCode(0);
        throw exp;
    }catch (CORBA::SystemException &e){
        FDS_ERROR("LpSoapRpcLink::disconnect : catch CORBA exception " << e);
        throw new FDSException::FDSStandardException ("Corba Exception accur while call disconnect on javalinkhost ",0);
    }catch(...){
        throw FDSException::FDSStandardException("internal unknown exception error",0);
    }
}

void FSCSOAPCommandSession::attachJavaLinkHost(){
    //find IOR file
    try{
        RWCString iorFile = getenv("FDS_IOR_DIR");
        iorFile += "/";

        iorFile += SOAPRPCLINK_S_IOR_FILENAME ;
        ifstream fin;
        fin.open(iorFile);
        char* iorString = new char[2048] ;
        memset(iorString,0,2048);
        fin >>iorString;
        cout<<"ior iorStringing :" <<iorString<<endl;
        fin.close();
    
        //string to object 
        CORBA::Object_var objV = FDSOrbix::string_to_object(iorString);
		delete[] iorString;
        if (CORBA::is_nil(objV))
        {
            FDS_DEBUG("[SoapRpcLink] CORBA::is_nil()");
            bool isExisted = true;
            try
            {
                if(objV->_non_existent() )    // e.g. invalid IOR
                {
                    isExisted = false;
                }
            }catch(...)
            {
                isExisted = false;
            }
            if(!isExisted)
            {
                FDS_DEBUG("[SoapRpcLink] _non_existent");
            }
            throw FDSException::FDSStandardException("ior string of JavaLinkHost is invalid",2002);
        }
        m_javaLinkHostPtr = JavaLinkSession::_narrow(objV);
    }catch(...){
        throw FDSException::FDSStandardException("SOAPRPCLINK: Failed in attaching the JavaLinkHost", 0);
    }
    //test nono_exist()
}
