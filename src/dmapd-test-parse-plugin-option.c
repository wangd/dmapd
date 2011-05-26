#include <check.h>
#include <gmodule.h>

#include "util.h"
#include "dmapd-daap-record.h"
#include "dmapd-daap-record-factory.h"
#include "dmapd-dmap-db-bdb.h"

START_TEST(test_dmapd_parse_plugin_option_simple)
{
	GHashTable *hash_table = g_hash_table_new (g_str_hash, g_str_equal);
	gchar *str = g_strdup ("gst");

	gchar *result = parse_plugin_option (str, hash_table);

	fail_unless (! strcmp (result, "gst"));

	g_hash_table_destroy (hash_table);
	g_free (str);
}
END_TEST

START_TEST(test_dmapd_parse_plugin_option_full)
{
	gchar *val;
	GHashTable *hash_table = g_hash_table_new (g_str_hash, g_str_equal);
	gchar *str = g_strdup ("gst:sink=apex,host=Host.local,port=5000,generation=1,protocol=1");

	gchar *result = parse_plugin_option (str, hash_table);

	fail_unless (! strcmp (result, "gst"));

	val = g_hash_table_lookup (hash_table, "sink");
	fail_unless (! strcmp (val, "apex"));

	val = g_hash_table_lookup (hash_table, "host");
	fail_unless (! strcmp (val, "Host.local"));

	val = g_hash_table_lookup (hash_table, "port");
	fail_unless (! strcmp (val, "5000"));

	val = g_hash_table_lookup (hash_table, "generation");
	fail_unless (! strcmp (val, "1"));

	val = g_hash_table_lookup (hash_table, "protocol");
	fail_unless (! strcmp (val, "1"));

	g_hash_table_destroy (hash_table);
	g_free (str);
}
END_TEST

Suite *dmapd_test_parse_plugin_option_suite(void)
{
	TCase *tc;
        Suite *s = suite_create("dmapd-test-parse-plugin-option-suite");

	tc = tcase_create("test_dmapd_parse_plugin_option_simple");
	tcase_add_test(tc, test_dmapd_parse_plugin_option_simple);
	suite_add_tcase(s, tc);

	tc = tcase_create("test_dmapd_parse_plugin_option_full");
	tcase_add_test(tc, test_dmapd_parse_plugin_option_full);
	suite_add_tcase(s, tc);

	return s;
}
