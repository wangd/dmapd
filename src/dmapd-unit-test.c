/*   FILE: dmapd-unit-test.c -- Unit tests
 * AUTHOR: W. Michael Petullo <mike@flyn.org>
 *   DATE: 01 January 2009 
 *
 * Copyright (c) 2009 W. Michael Petullo <new@flyn.org>
 * All rights reserved.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <check.h>
#include <glib.h>
#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libdmapsharing/dmap.h>

#include "util.h"
#include "dmapd-daap-record.h"
#include "dmapd-daap-record-factory.h"
#include "dmapd-dmap-db-bdb.h"

static Suite *dmapd_suite(void)
{
/*
        Suite *s = suite_create("dmapd");

	TCase *tc_dmapd_daap_record_add_lookup = tcase_create("test_dmapd_daap_record_add_lookup");
	tcase_add_test(tc_dmapd_daap_record_add_lookup, test_dmapd_daap_record_add_lookup);
	suite_add_tcase(s, tc_dmapd_daap_record_add_lookup);

	return s;
*/
}

int main(void)
{
/*
	int nf;

	g_type_init ();

	Suite *s = dmapd_suite();
	SRunner *sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
*/
}
