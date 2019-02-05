/**
 * XMPP - libpurple transport
 *
 * Copyright (C) 2009, Jan Kaluza <hanzz@soc.pidgin.im>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#include "transport/Logging.h"
#include "transport/LocalBuddy.h"
#include "transport/User.h"
#include "transport/RosterManager.h"
#include "transport/Transport.h"
#include "transport/Frontend.h"

DEFINE_LOGGER(logger, "LocalBuddy");

namespace Transport {

LocalBuddy::LocalBuddy(RosterManager *rosterManager, long id, const std::string &name, const std::string &alias, const std::vector<std::string> &groups, BuddyFlag flags) : Buddy(rosterManager, id, flags) {
	m_status = Swift::StatusShow::None;
	m_alias = alias;
	m_name = name;
	m_groups = groups;
	LOG4CXX_DEBUG(logger, "Creating LocalBuddy: name=" << m_name << ", status=" << m_status.getType());
	try {
		generateJID();
	} catch (...) {
	}
}

LocalBuddy::~LocalBuddy() {
}

void LocalBuddy::setStatus(const Swift::StatusShow &status, const std::string &statusMessage) {
	LOG4CXX_DEBUG(logger, "setStatus=" << status.getType() << ", message=" << statusMessage);
	bool changed = ((m_status.getType() != status.getType()) || (m_statusMessage != statusMessage));
	if (changed) {
		LOG4CXX_DEBUG(logger, "status changed, sending presence; old=" << m_status.getType() << ", old message=" << m_statusMessage);
		m_status = status;
		m_statusMessage = statusMessage;
		sendPresence();
	} else {
		LOG4CXX_DEBUG(logger, "status unchanged, old=" << m_status.getType() << ", old message=" << m_statusMessage);
	}
}

void LocalBuddy::setIconHash(const std::string &iconHash) {
	LOG4CXX_DEBUG(logger, "setIconHash='" << iconHash << "'");
	bool changed = m_iconHash != iconHash;
	if (changed) {
		LOG4CXX_DEBUG(logger, "iconHash changed, sending presence; old='" << m_iconHash << "'");
		m_iconHash = iconHash;
		getRosterManager()->storeBuddy(this);
		sendPresence();
	} else {
		LOG4CXX_DEBUG(logger, "iconHash unchanged, old iconHash=" << m_iconHash);
	}
}

bool LocalBuddy::setName(const std::string &name) {
	if (name == m_name) {
		return true;
	}
	std::string oldName = name;
	m_name = name;
	try {
		generateJID();
		return m_jid.isValid();
	} catch (...) {
		m_name = oldName;
		return false;
	}
}

void LocalBuddy::setAlias(const std::string &alias) {
	bool changed = m_alias != alias;
	m_alias = alias;

	if (changed) {
		getRosterManager()->doUpdateBuddy(this);
		getRosterManager()->storeBuddy(this);
	}
}

void LocalBuddy::setGroups(const std::vector<std::string> &groups) {
	bool changed = m_groups.size() != groups.size();
	if (!changed) {
		for (unsigned i = 0; i != m_groups.size(); i++) {
			if (m_groups[i] != groups[i]) {
				changed = true;
				break;
			}
		}
	}

	m_groups = groups;
	if (changed) {
		getRosterManager()->doUpdateBuddy(this);
		getRosterManager()->storeBuddy(this);
	}
}

bool LocalBuddy::getStatus(Swift::StatusShow &status, std::string &statusMessage) {
	if (getRosterManager()->getUser()->getComponent()->getFrontend()->isRawXMLEnabled()) {
		return false;
	}
	status = m_status;
	statusMessage = m_statusMessage;
	return true;
}

}
