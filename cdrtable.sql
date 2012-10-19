CREATE TABLE `filter_ip` (
  `id` int(32) NOT NULL AUTO_INCREMENT,
  `ip` int(32) unsigned DEFAULT NULL,
  `mask` int(8) DEFAULT NULL,
  `direction` tinyint(8) DEFAULT '0',
  `rtp` tinyint(1) DEFAULT '0',
  `sip` tinyint(1) DEFAULT '0',
  `register` tinyint(1) DEFAULT '0',
  `graph` tinyint(1) DEFAULT '0',
  `wav` tinyint(1) DEFAULT '0',
  `note` text,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `filter_telnum` (
  `id` int(32) NOT NULL AUTO_INCREMENT,
  `prefix` bigint(32) unsigned DEFAULT NULL,
  `fixed_len` int(32) unsigned DEFAULT '0',
  `direction` tinyint(8) DEFAULT '0',
  `rtp` tinyint(1) DEFAULT '0',
  `sip` tinyint(1) DEFAULT '0',
  `register` tinyint(1) DEFAULT '0',
  `graph` tinyint(1) DEFAULT '0',
  `wav` tinyint(1) DEFAULT '0',
  `note` text,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `cdr_sip_response` (
  `id` smallint(5) unsigned NOT NULL AUTO_INCREMENT,
  `lastSIPresponse` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `lastSIPresponse` (`lastSIPresponse`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `cdr_ua` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `ua` varchar(512) DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `ua` (`ua`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=COMPRESSED;

CREATE TABLE `cdr` (
  `ID` int(32) unsigned NOT NULL AUTO_INCREMENT,
  `calldate` datetime NOT NULL,
  `callend` datetime NOT NULL,
  `duration` mediumint(8) unsigned DEFAULT NULL,
  `connect_duration` mediumint(8) unsigned DEFAULT NULL,
  `progress_time` mediumint(8) unsigned DEFAULT NULL,
  `first_rtp_time` mediumint(8) unsigned DEFAULT NULL,
  `caller` varchar(255) DEFAULT NULL,
  `caller_domain` varchar(255) DEFAULT NULL,
  `caller_reverse` varchar(255) DEFAULT NULL,
  `callername` varchar(255) DEFAULT NULL,
  `callername_reverse` varchar(255) DEFAULT NULL,
  `called` varchar(255) DEFAULT NULL,
  `called_domain` varchar(255) DEFAULT NULL,
  `called_reverse` varchar(255) DEFAULT NULL,
  `sipcallerip` int(10) unsigned DEFAULT NULL,
  `sipcalledip` int(10) unsigned DEFAULT NULL,
  `custom_header1` varchar(255) DEFAULT NULL,
  `whohanged` enum('caller','callee') DEFAULT NULL,
  `bye` tinyint(3) unsigned DEFAULT NULL,
  `lastSIPresponse_id` smallint(5) unsigned DEFAULT NULL,
  `lastSIPresponseNum` smallint(5) unsigned DEFAULT NULL,
  `sighup` tinyint(4) DEFAULT NULL,
  `a_index` tinyint(4) DEFAULT NULL,
  `b_index` tinyint(4) DEFAULT NULL,
  `a_payload` int(11) DEFAULT NULL,
  `b_payload` int(11) DEFAULT NULL,
  `a_saddr` int(10) unsigned DEFAULT NULL,
  `b_saddr` int(10) unsigned DEFAULT NULL,
  `a_received` mediumint(8) unsigned DEFAULT NULL,
  `b_received` mediumint(8) unsigned DEFAULT NULL,
  `a_lost` mediumint(8) unsigned DEFAULT NULL,
  `b_lost` mediumint(8) unsigned DEFAULT NULL,
  `a_ua_id` int(10) unsigned DEFAULT NULL,
  `b_ua_id` int(10) unsigned DEFAULT NULL,
  `a_avgjitter_mult10` mediumint(8) unsigned DEFAULT NULL,
  `b_avgjitter_mult10` mediumint(8) unsigned DEFAULT NULL,
  `a_maxjitter` smallint(5) unsigned DEFAULT NULL,
  `b_maxjitter` smallint(5) unsigned DEFAULT NULL,
  `a_sl1` mediumint(8) unsigned DEFAULT NULL,
  `a_sl2` mediumint(8) unsigned DEFAULT NULL,
  `a_sl3` mediumint(8) unsigned DEFAULT NULL,
  `a_sl4` mediumint(8) unsigned DEFAULT NULL,
  `a_sl5` mediumint(8) unsigned DEFAULT NULL,
  `a_sl6` mediumint(8) unsigned DEFAULT NULL,
  `a_sl7` mediumint(8) unsigned DEFAULT NULL,
  `a_sl8` mediumint(8) unsigned DEFAULT NULL,
  `a_sl9` mediumint(8) unsigned DEFAULT NULL,
  `a_sl10` mediumint(8) unsigned DEFAULT NULL,
  `a_d50` mediumint(8) unsigned DEFAULT NULL,
  `a_d70` mediumint(8) unsigned DEFAULT NULL,
  `a_d90` mediumint(8) unsigned DEFAULT NULL,
  `a_d120` mediumint(8) unsigned DEFAULT NULL,
  `a_d150` mediumint(8) unsigned DEFAULT NULL,
  `a_d200` mediumint(8) unsigned DEFAULT NULL,
  `a_d300` mediumint(8) unsigned DEFAULT NULL,
  `b_sl1` mediumint(8) unsigned DEFAULT NULL,
  `b_sl2` mediumint(8) unsigned DEFAULT NULL,
  `b_sl3` mediumint(8) unsigned DEFAULT NULL,
  `b_sl4` mediumint(8) unsigned DEFAULT NULL,
  `b_sl5` mediumint(8) unsigned DEFAULT NULL,
  `b_sl6` mediumint(8) unsigned DEFAULT NULL,
  `b_sl7` mediumint(8) unsigned DEFAULT NULL,
  `b_sl8` mediumint(8) unsigned DEFAULT NULL,
  `b_sl9` mediumint(8) unsigned DEFAULT NULL,
  `b_sl10` mediumint(8) unsigned DEFAULT NULL,
  `b_d50` mediumint(8) unsigned DEFAULT NULL,
  `b_d70` mediumint(8) unsigned DEFAULT NULL,
  `b_d90` mediumint(8) unsigned DEFAULT NULL,
  `b_d120` mediumint(8) unsigned DEFAULT NULL,
  `b_d150` mediumint(8) unsigned DEFAULT NULL,
  `b_d200` mediumint(8) unsigned DEFAULT NULL,
  `b_d300` mediumint(8) unsigned DEFAULT NULL,
  `a_mos_f1_mult10` tinyint(3) unsigned DEFAULT NULL,
  `a_mos_f2_mult10` tinyint(3) unsigned DEFAULT NULL,
  `a_mos_adapt_mult10` tinyint(3) unsigned DEFAULT NULL,
  `b_mos_f1_mult10` tinyint(3) unsigned DEFAULT NULL,
  `b_mos_f2_mult10` tinyint(3) unsigned DEFAULT NULL,
  `b_mos_adapt_mult10` tinyint(3) unsigned DEFAULT NULL,
  `a_rtcp_loss` smallint(5) unsigned DEFAULT NULL,
  `a_rtcp_maxfr` smallint(5) unsigned DEFAULT NULL,
  `a_rtcp_avgfr_mult10` smallint(5) unsigned DEFAULT NULL,
  `a_rtcp_maxjitter` smallint(5) unsigned DEFAULT NULL,
  `a_rtcp_avgjitter_mult10` smallint(5) unsigned DEFAULT NULL,
  `b_rtcp_loss` smallint(5) unsigned DEFAULT NULL,
  `b_rtcp_maxfr` smallint(5) unsigned DEFAULT NULL,
  `b_rtcp_avgfr_mult10` smallint(5) unsigned DEFAULT NULL,
  `b_rtcp_maxjitter` smallint(5) unsigned DEFAULT NULL,
  `b_rtcp_avgjitter_mult10` smallint(5) unsigned DEFAULT NULL,
  `payload` int(11) DEFAULT NULL,
  `jitter_mult10` mediumint(8) unsigned DEFAULT NULL,
  `mos_min_mult10` tinyint(3) unsigned DEFAULT NULL,
  `a_mos_min_mult10` tinyint(3) unsigned DEFAULT NULL,
  `b_mos_min_mult10` tinyint(3) unsigned DEFAULT NULL,
  `packet_loss_perc_mult1000` mediumint(8) unsigned DEFAULT NULL,
  `a_packet_loss_perc_mult1000` mediumint(8) unsigned DEFAULT NULL,
  `b_packet_loss_perc_mult1000` mediumint(8) unsigned DEFAULT NULL,
  `delay_sum` mediumint(8) unsigned DEFAULT NULL,
  `a_delay_sum` mediumint(8) unsigned DEFAULT NULL,
  `b_delay_sum` mediumint(8) unsigned DEFAULT NULL,
  `delay_avg_mult100` mediumint(8) unsigned DEFAULT NULL,
  `a_delay_avg_mult100` mediumint(8) unsigned DEFAULT NULL,
  `b_delay_avg_mult100` mediumint(8) unsigned DEFAULT NULL,
  `delay_cnt` mediumint(8) unsigned DEFAULT NULL,
  `a_delay_cnt` mediumint(8) unsigned DEFAULT NULL,
  `b_delay_cnt` mediumint(8) unsigned DEFAULT NULL,
  `rtcp_avgfr_mult10` smallint(5) unsigned DEFAULT NULL,
  `rtcp_avgjitter_mult10` smallint(5) unsigned DEFAULT NULL,
  `lost` mediumint(8) unsigned DEFAULT NULL,
  `id_sensor` int(10) unsigned DEFAULT NULL,
  PRIMARY KEY (`ID`),
  KEY `calldate` (`calldate`),
  KEY `callend` (`callend`),
  KEY `duration` (`duration`),
  KEY `source` (`caller`),
  KEY `source_reverse` (`caller_reverse`),
  KEY `destination` (`called`),
  KEY `destination_reverse` (`called_reverse`),
  KEY `callername` (`callername`),
  KEY `callername_reverse` (`callername_reverse`),
  KEY `sipcallerip` (`sipcallerip`),
  KEY `sipcalledip` (`sipcalledip`),
  KEY `lastSIPresponseNum` (`lastSIPresponseNum`),
  KEY `bye` (`bye`),
  KEY `a_saddr` (`a_saddr`),
  KEY `b_saddr` (`b_saddr`),
  KEY `a_lost` (`a_lost`),
  KEY `b_lost` (`b_lost`),
  KEY `a_maxjitter` (`a_maxjitter`),
  KEY `b_maxjitter` (`b_maxjitter`),
  KEY `a_rtcp_loss` (`a_rtcp_loss`),
  KEY `a_rtcp_maxfr` (`a_rtcp_maxfr`),
  KEY `a_rtcp_maxjitter` (`a_rtcp_maxjitter`),
  KEY `b_rtcp_loss` (`b_rtcp_loss`),
  KEY `b_rtcp_maxfr` (`b_rtcp_maxfr`),
  KEY `b_rtcp_maxjitter` (`b_rtcp_maxjitter`),
  KEY `a_ua_id` (`a_ua_id`),
  KEY `b_ua_id` (`b_ua_id`),
  KEY `a_avgjitter_mult10` (`a_avgjitter_mult10`),
  KEY `b_avgjitter_mult10` (`b_avgjitter_mult10`),
  KEY `a_rtcp_avgjitter_mult10` (`a_rtcp_avgjitter_mult10`),
  KEY `b_rtcp_avgjitter_mult10` (`b_rtcp_avgjitter_mult10`),
  KEY `lastSIPresponse_id` (`lastSIPresponse_id`),
  KEY `payload` (`payload`),
  KEY `id_sensor` (`id_sensor`),
  CONSTRAINT `cdr_ibfk_1` FOREIGN KEY (`lastSIPresponse_id`) REFERENCES `cdr_sip_response` (`id`) ON UPDATE CASCADE,
  CONSTRAINT `cdr_ibfk_2` FOREIGN KEY (`a_ua_id`) REFERENCES `cdr_ua` (`id`) ON UPDATE CASCADE,
  CONSTRAINT `cdr_ibfk_3` FOREIGN KEY (`b_ua_id`) REFERENCES `cdr_ua` (`id`) ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=COMPRESSED;

CREATE TABLE `cdr_next` (
  `cdr_ID` int(10) unsigned NOT NULL,
  `custom_header1` varchar(255) DEFAULT NULL,
  `fbasename` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`cdr_ID`),
  KEY `fbasename` (`fbasename`),
  CONSTRAINT `cdr_next_ibfk_1` FOREIGN KEY (`cdr_ID`) REFERENCES `cdr` (`ID`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=COMPRESSED;

CREATE TABLE `register` (
  `ID` int(32) unsigned NOT NULL AUTO_INCREMENT,
  `calldate` datetime NOT NULL,
  `sipcallerip` int(32) unsigned NOT NULL,
  `sipcalledip` int(32) unsigned NOT NULL,
  `from_num` varchar(255) NULL DEFAULT NULL,
  `from_name` varchar(255) NULL DEFAULT NULL,
  `from_domain` varchar(255) NULL DEFAULT NULL,
  `to_num` varchar(255) NULL DEFAULT NULL,
  `to_domain` varchar(255) NULL DEFAULT NULL,
  `contact_num` varchar(255) NULL DEFAULT NULL,
  `contact_domain` varchar(255) NULL DEFAULT NULL,
  `digestusername` varchar(255) NULL DEFAULT NULL,
  `digestrealm` varchar(255) NULL DEFAULT NULL,
  `expires`	mediumint NULL DEFAULT NULL,
  `expires_at`  datetime NULL DEFAULT NULL,
  `state`  tinyint unsigned NULL DEFAULT NULL,
  `ua_id` int(10) unsigned DEFAULT NULL,
  PRIMARY KEY (`ID`),
  KEY `calldate` (`calldate`),
  KEY `sipcallerip` (`sipcallerip`),
  KEY `sipcalledip` (`sipcalledip`),
  KEY `from_num` (`from_num`),
  KEY `digestusername` (`digestusername`)
) ENGINE=MEMORY DEFAULT CHARSET=latin1 ROW_FORMAT=COMPRESSED;

CREATE TABLE `register_state` (
  `ID` int(32) unsigned NOT NULL AUTO_INCREMENT,
  `created_at` datetime NOT NULL,
  `sipcallerip` int(32) unsigned NOT NULL,
  `from_num` varchar(255) NULL DEFAULT NULL,
  `to_num` varchar(255) NULL DEFAULT NULL,
  `contact_num` varchar(255) NULL DEFAULT NULL,
  `contact_domain` varchar(255) NULL DEFAULT NULL,
  `digestusername` varchar(255) NULL DEFAULT NULL,
  `expires` mediumint NULL DEFAULT NULL,
  `state` tinyint unsigned NULL DEFAULT NULL,
  `ua_id` int(10) unsigned DEFAULT NULL,
  PRIMARY KEY (`ID`),
  KEY `created_at` (`created_at`),
  KEY `sipcallerip` (`sipcallerip`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=COMPRESSED;

CREATE TABLE `register_failed` (
  `ID` int(32) unsigned NOT NULL AUTO_INCREMENT,
  `counter` int DEFAULT 0,
  `created_at` datetime NOT NULL,
  `sipcallerip` int(32) unsigned NOT NULL,
  `from_num` varchar(255) NULL DEFAULT NULL,
  `to_num` varchar(255) NULL DEFAULT NULL,
  `contact_num` varchar(255) NULL DEFAULT NULL,
  `contact_domain` varchar(255) NULL DEFAULT NULL,
  `digestusername` varchar(255) NULL DEFAULT NULL,
  `ua_id` int(10) unsigned DEFAULT NULL,
  PRIMARY KEY (`ID`),
  KEY `created_at` (`created_at`),
  KEY `sipcallerip` (`sipcallerip`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=COMPRESSED;

