#ifndef MessageFacility_Extensions_ELarchive_h
#define MessageFacility_Extensions_ELarchive_h


// ----------------------------------------------------------------------
//
// ELarchive	is a subclass of ELdestination representing the standard
//		provided destination.
//
// 7/8/98 mf	Created file.
// 6/17/99 jvr	Made output format options available for ELdestControl only
// 7/2/99 jvr	Added separate/attachTime, Epilogue, and Serial options
// 2/22/00 mf	Changed myDetX to myOutputX (to avoid future puzzlement!)
//		and added ELarchive(ox) to cacilitate inherited classes.
// 6/7/00 web	Consolidated ELarchive/X; add filterModule()
// 6/14/00 web	Declare classes before granting friendship; remove using
// 10/4/00 mf	add excludeModule()
//  4/4/01 mf 	Removed moduleOfInterest and moduleToExclude, in favor
//		of using base class method.
//  6/23/03 mf  changeFile(), flush() 
//  6/11/07 mf  changed default for emitAtStart to false  
//
// ----------------------------------------------------------------------

#include "messagefacility/MessageService/ELdestination.h"

#include "messagefacility/MessageLogger/ELstring.h"
#include "messagefacility/MessageLogger/ELextendedID.h"

#include "fhiclcpp/ParameterSet.h"

#include "boost/shared_ptr.hpp"

namespace mf {       


// ----------------------------------------------------------------------
// prerequisite classes:
// ----------------------------------------------------------------------

class ErrorObj;
namespace service {       

class ELdestControl;


// ----------------------------------------------------------------------
// ELarchive:
// ----------------------------------------------------------------------

class ELarchive : public ELdestination  {

  friend class ELdestControl;

public:

  // ---  Birth/death:
  //
  ELarchive();
  ELarchive( std::string const & name_, fhicl::ParameterSet const & pset_ );
  //ELarchive( std::ostream & os, bool emitAtStart = false );	// 6/11/07 mf
  //ELarchive( const ELstring & fileName, bool emitAtStart = false );
  ELarchive( const ELarchive & orig );
  virtual ~ELarchive();

  // ---  Methods invoked by the ELadministrator:
  //
public:
  virtual
  ELarchive *
  clone() const;
  // Used by attach() to put the destination on the ELadministrators list
                //-| There is a note in Design Notes about semantics
                //-| of copying a destination onto the list:  ofstream
                //-| ownership is passed to the new copy.

  virtual bool log( const mf::ErrorObj & msg );

  // ---  Methods invoked through the ELdestControl handle:
  //
protected:
    // trivial clearSummary(), wipe(), zero() from base class
    // trivial three summary(..) from base class

  // ---  Data affected by methods of specific ELdestControl handle:
  //
protected:
    // ELarchive uses the generic ELdestControl handle

  // ---  Internal Methods -- Users should not invoke these:
  //
protected:
  virtual void emit( const ELstring & s, bool nl=false );

  virtual void suppressTime();        virtual void includeTime();
  virtual void suppressMillisecond(); virtual void includeMillisecond();
  virtual void suppressModule();      virtual void includeModule();
  virtual void suppressSubroutine();  virtual void includeSubroutine();
  virtual void suppressText();        virtual void includeText();
  virtual void suppressContext();     virtual void includeContext();
  virtual void suppressSerial();      virtual void includeSerial();
  virtual void useFullContext();      virtual void useContext();
  virtual void separateTime();        virtual void attachTime();
  virtual void separateEpilogue();    virtual void attachEpilogue();

  virtual void summarization ( const ELstring & fullTitle
                             , const ELstring & sumLines );
			     
  virtual void changeFile (std::ostream & os);
  virtual void changeFile (const ELstring & filename);
  virtual void flush(); 				       


protected:
  // --- member data:
  //
  boost::shared_ptr<std::ostream> os;
  int                             charsOnLine;
  mf::ELextendedID               xid;

  bool wantTimestamp
  ,    wantMillisecond
  ,    wantModule
  ,    wantSubroutine
  ,    wantText
  ,    wantSomeContext
  ,    wantSerial
  ,    wantFullContext
  ,    wantTimeSeparate
  ,    wantEpilogueSeparate
  ,    preambleMode
  ;

  // --- Verboten method:
  //
  ELarchive & operator=( const ELarchive & orig );

};  // ELarchive


// ----------------------------------------------------------------------


}        // end of namespace service
}        // end of namespace mf


#endif // MessageFacility_Extensions_ELarchive_h
