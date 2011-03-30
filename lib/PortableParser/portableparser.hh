/*
 *  Little Green BATS (2008), AI department, University of Groningen
 *
 *  Authors: 	Martin Klomp (martin@ai.rug.nl)
 *		Mart van de Sanden (vdsanden@ai.rug.nl)
 *		Sander van Dijk (sgdijk@ai.rug.nl)
 *		A. Bram Neijt (bneijt@gmail.com)
 *		Matthijs Platje (mplatje@gmail.com)
 *
 *	All students of AI at the University of Groningen
 *  at the time of writing. See: http://www.ai.rug.nl/
 *
 *  Date: 	November 1, 2008
 *
 *  Website:	http://www.littlegreenbats.nl
 *
 *  Comment:	Please feel free to contact us if you have any 
 *		problems or questions about the code.
 *
 *
 *  License: 	This program is free software; you can redistribute 
 *		it and/or modify it under the terms of the GNU General
 *		Public License as published by the Free Software 
 *		Foundation; either version 3 of the License, or (at 
 *		your option) any later version.
 *
 *   		This program is distributed in the hope that it will
 *		be useful, but WITHOUT ANY WARRANTY; without even the
 *		implied warranty of MERCHANTABILITY or FITNESS FOR A
 *		PARTICULAR PURPOSE.  See the GNU General Public
 *		License for more details.
 *
 *   		You should have received a copy of the GNU General
 *		Public License along with this program; if not, write
 *		to the Free Software Foundation, Inc., 59 Temple Place - 
 *		Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifndef SC_PORTABLEPARSER_HH_
#define SC_PORTABLEPARSER_HH_

#include "../Parser/parser.hh"
#include <stdexcept>
#include <iostream>

namespace sc {

  /**\brief Reentrant Communication Parser
   * 
   *  A reentrant version of The Parser (tm)...
   *
   *  It is a little bit slower compared to Parser, but is still really fast.
   *
   *  The parser is used to parse the communication messages (the lisp like language)
   *  to a Predicate (tree like structure @see Predicate).
   *
   *  Because the normal Parser is broken at the moment, this is the only parser available.
   *
   */
  class PortableParser {
    
    static Parser::ParseEntry s_initializerState[256];

    // Lexer state table pointers.
    Parser::ParseEntry *d_pCurrentState;

    // The stack (there are faster stacks than this one..).
    PredicateStack d_stck;

    std::string d_tokenStr;

    bool d_done;

    //    std::stringstream d_err;

    PortableParser(Parser const &); // NI
    PortableParser &operator=(Parser const &); // NI

  public:

    PortableParser() { reset(); }

    /**
     *  Resets the portable parser. This always needs to be done
     *  before beginning to parse something.
     */
    void reset()
    {
      d_done = false;
      d_pCurrentState = s_initializerState;
      d_stck = PredicateStack();
      boost::shared_ptr<Predicate> pred(new Predicate(Predicate::type_list));
      d_stck.push(pred);
    }

    /**
     *  Inputs a single character to the parser.
     *  @returns true when we have finished parsing a line.
     */
    inline bool parse(char chr)
    {
      d_pCurrentState = reinterpret_cast<Parser::ParseEntry*>(d_pCurrentState->value) + chr;

      switch (d_pCurrentState->action) {

      case Parser::Shift:
        d_tokenStr += chr;
        return false;

      case Parser::ShiftNull:
        d_tokenStr.clear();
        return false;

      case Parser::PushPred:
      {
        boost::shared_ptr<Predicate> pred(new Predicate(d_tokenStr,Predicate::type_node));
        d_stck.push(d_stck.top()->push(pred));
        d_tokenStr.clear();
        return false;
      }

      case Parser::AddToPred:
      {
        boost::shared_ptr<Predicate> pred(new Predicate(d_tokenStr,Predicate::type_node));
        d_stck.top()->push(pred);
        d_tokenStr.clear();
        return false;
      }

      case Parser::ClosePred:
        d_stck.pop();
        if (d_stck.empty())
          throw std::runtime_error("parse error, to many right parentises");
        d_tokenStr.clear();
        return false;

      case Parser::ClosePredEnd:
        d_stck.pop();
      case Parser::End:
        if (d_stck.empty())
          throw std::runtime_error("parse error, to many right parentises");
        else if (d_stck.size() > 1) {
          for (unsigned i = d_stck.size(); i; --i) {
            d_stck.pop();
          }
          throw std::runtime_error("parse error, predicate is not closed");
        }
        d_done = true;
        return true;

      case Parser::Error:
        throw std::runtime_error(std::string()+"unknown parser or lexer error (current char: '"+static_cast<char>(chr)+"')");

      case Parser::Warning:
        throw std::runtime_error(std::string()+"parler warning (current char: '"+static_cast<char>(chr)+"')");

      };

      d_done = true;
      return true;
    }


    /**
     *  Parses up to len bytes from buf.
     *
     *  @returns the number of parsed bytes.
     *  @throws BatsException*
     */
    inline unsigned parse(char const *buf, unsigned len)
    {
      char const *i;
      try
      {
        for (i = buf; i < buf + len; ++i) {
          if (!*i) {
            throw std::runtime_error("Found a zero in the parser buffer (this usually means that something went wrong with the agent<->server communication).");
          }
          if (parse(*i))
            return i-buf;
        }
      }
      catch (std::runtime_error e)
      {
        std::cerr << "(PortableParser) Error parsing message: " << e.what() << " at char " << (int)(i < buf) << std::endl;
        throw std::runtime_error(e.what());
      }
      
      return len;
    }

    /**
     *  Parses a complete line and returns the predicate.
     *
     *  @returns the parsed predicate.
     */
    inline boost::shared_ptr<Predicate> parseLine(std::string const &data)
    {
      unsigned count = 0;
      while (count < data.length()) {
        count += parse(reinterpret_cast<char const *>(data.c_str())+count,data.length()-count+1);
      }
      return d_stck.top();
    }

    /**
     *  @returns true when we have parsed a exactly one predicate
     *           no more and no less.
     */
    inline bool endOfPred() const
    {
      return (d_stck.size() == 1);
    }

    /**
     *  @returns true if a complete line had been parsed.
     */
    inline bool done() const
    {
      return d_done;
    }
    
    /**
     *  @returns the parsed predicate.
     */
    boost::shared_ptr<Predicate> getPredicate() const
    {
      return d_stck.top();
    }

  };

};

#endif // SC_PORTABLEPARSER_HH_
