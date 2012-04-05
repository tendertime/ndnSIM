/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2011 University of California, Los Angeles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#include "ccnb-parser-content-object-visitor.h"
#include "ccnb-parser-name-components-visitor.h"
#include "ccnb-parser-non-negative-integer-visitor.h"
#include "ccnb-parser-timestamp-visitor.h"

#include "../syntax-tree/ccnb-parser-block.h"
#include "../syntax-tree/ccnb-parser-dtag.h"

#include "ns3/ccnx-name-components.h"
#include "ns3/assert.h"
#include "ns3/log.h"

#include "ns3/ccnx-content-object-header.h"

#include <boost/foreach.hpp>

NS_LOG_COMPONENT_DEFINE ("CcnbParserContentObjectVisitor");

namespace ns3 {
namespace CcnbParser {

// We don't really care about any other fields
void
ContentObjectVisitor::visit (Dtag &n, boost::any param/*should be CcnxContentObjectHeader* */)
{
  // uint32_t n.m_dtag;
  // std::list<Ptr<Block> > n.m_nestedBlocks;
  static NameComponentsVisitor nameComponentsVisitor;
  static NonNegativeIntegerVisitor nonNegativeIntegerVisitor;
  static TimestampVisitor          timestampVisitor;
  
  CcnxContentObjectHeader &contentObject = *(boost::any_cast<CcnxContentObjectHeader*> (param));
  
  switch (n.m_dtag)
    {
    case CCN_DTAG_ContentObject:
      // process nested blocks
      BOOST_FOREACH (Ptr<Block> block, n.m_nestedTags)
        {
          block->accept (*this, param);
        }
      break;
    case CCN_DTAG_Name:
      {
        // process name components
        Ptr<CcnxNameComponents> name = Create<CcnxNameComponents> ();
        
        BOOST_FOREACH (Ptr<Block> block, n.m_nestedTags)
          {
            block->accept (nameComponentsVisitor, &(*name));
          }
        contentObject.SetName (name);
        break;
      }

    case CCN_DTAG_Signature: // ignoring
      break;

    case CCN_DTAG_SignedInfo:
      // process nested blocks
      BOOST_FOREACH (Ptr<Block> block, n.m_nestedTags)
        {
          block->accept (*this, param);
        }      
      break;
      
    case CCN_DTAG_Timestamp:
      NS_LOG_DEBUG ("Timestamp");
      if (n.m_nestedTags.size()!=1) // should be exactly one UDATA inside this tag
        throw CcnbDecodingException ();

      contentObject.SetTimestamp (
               boost::any_cast<Time> (
                                      (*n.m_nestedTags.begin())->accept(
                                                                        timestampVisitor
                                                                        )));
      break;

    case CCN_DTAG_FreshnessSeconds:
      NS_LOG_DEBUG ("FreshnessSeconds");
      
      if (n.m_nestedTags.size()!=1) // should be exactly one UDATA inside this tag
        throw CcnbDecodingException ();
      contentObject.SetFreshness (
          Seconds (
               boost::any_cast<uint32_t> (
                                          (*n.m_nestedTags.begin())->accept(
                                                                           nonNegativeIntegerVisitor
                                                                           ))));
      
      break;
      
    case CCN_DTAG_Content: // !!! HACK
      // This hack was necessary for memory optimizations (i.e., content is virtual payload)
      NS_ASSERT_MSG (n.m_nestedTags.size() == 0, "Parser should have stopped just after processing <Content> tag");
      break;
      
    default: // ignore all other stuff
      break;
    }
}

} // namespace CcnbParser
} // namespace ns3
