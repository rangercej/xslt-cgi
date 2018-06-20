<?xml version="1.0"?>

<!-- This transform does nothing of interest. It copies the input straight to
the output, overriding the <title> element. Expects the input to be a XHTML
file (well, will work for any XML file, but I wrote it as an example for 
XHTML :-) -->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="xml" />

	<!-- Essentially the entry point; match the root node of the document -->
	<xsl:template match="/">
		<xsl:apply-templates />
	</xsl:template>

	<!-- This template matches title, and replaces it with its own title -->
	<xsl:template match="/html/head/title">
		<title>The title has been overridden!</title>
	</xsl:template>

	<!-- Default catch-all; this recursively goes through the document. Every
	element bar <title> will be caught by this rule, which basically copies
	the subject node and its attributes, then calls apply-templates on the
	nodes' children. -->
	<xsl:template match="*">
		<xsl:copy>
			<xsl:copy-of select="@*" />
			<xsl:apply-templates />
		</xsl:copy>
	</xsl:template>

</xsl:stylesheet>
