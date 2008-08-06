<?xml version="1.0" encoding="iso-8859-1" ?>

<!-- Basic informations -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output version="4.01" method="html" encoding="iso-8859-2"/>

<!-- Header is first thing in Section-N -->
<xsl:template match="Header">
</xsl:template>

<xsl:template match="example">
<p><b>Pøíklad:</b> <xsl:apply-templates/></p>
</xsl:template>


<xsl:template match="Section-1">
<h1><xsl:value-of select="Header"/></h1>
<xsl:apply-templates/>
</xsl:template>

<xsl:template match="Section-2">
<H2><xsl:value-of select="Header"/></H2>
<xsl:apply-templates/>
</xsl:template>

<xsl:template match="Section-3">
<H3><xsl:value-of select="Header"/></H3>
<xsl:apply-templates/>
</xsl:template>

<xsl:template match="Section-4">
<H4><xsl:value-of select="Header"/></H4>
<xsl:apply-templates/>
</xsl:template>


<xsl:template match="CDK-desc">
<h2 class='function'><xsl:value-of select="@name"/></h2>
<img src="../../images/delphi.gif" width="44"
	 height="16" border="0" alt="Delphi / Kylix" align="middle" hspace="2"/>
<xsl:apply-templates/>
</xsl:template>

<xsl:template match="unit"><br/><br/><h3>Unit</h3><xsl:apply-templates/></xsl:template>
<xsl:template match="properties"><br/><br/><h2>Vlastnosti komponenty</h2><xsl:apply-templates/></xsl:template>
<xsl:template match="methods"><br/><br/><h2>Metody komponenty</h2><xsl:apply-templates/></xsl:template>
<xsl:template match="events"><br/><br/><h2>Události komponenty</h2><xsl:apply-templates/></xsl:template>
<xsl:template match="pme"><div class="pme"><xsl:apply-templates/></div></xsl:template>

<xsl:template match="dotnet-desc">
<h2 class='function'><xsl:value-of select="@name"/></h2>
<img src="../../images/csharp.gif" width="44"
	 height="16" border="0" alt="C#" align="middle" hspace="2"/>
<xsl:apply-templates/>
</xsl:template>

<xsl:template match="dn-unit"><br/><br/><h3>Assembly</h3><xsl:apply-templates/></xsl:template>
<xsl:template match="dn-interface"><br/><br/><h3>Implementuje tøídy a rozhraní</h3><xsl:apply-templates/></xsl:template>
<xsl:template match="dn-note"><br/><br/><h3>Poznámka:</h3><xsl:apply-templates/></xsl:template>
<xsl:template match="dn-properties"><br/><br/><h2>Vlastnosti</h2><xsl:apply-templates/></xsl:template>
<xsl:template match="dn-methods"><br/><br/><h2>Metody</h2><xsl:apply-templates/></xsl:template>
<xsl:template match="dn-const"><br/><br/><h2>Konstanty</h2><xsl:apply-templates/></xsl:template>
<xsl:template match="dn-pme"><div class="pme"><xsl:apply-templates/></div></xsl:template>
<xsl:template match="dn-ctor"><br/><br/><h2>Konstruktor</h2><xsl:apply-templates/></xsl:template>

<xsl:template match="function-desc">
<h2 class='function'><xsl:value-of select="@name"/></h2>
<xsl:for-each select="decl">
<xsl:choose>
<xsl:when test="self::node()[@lang='ipj']">
<img src="../../images/ipj.gif" width="44"
	 height="16" border="0" alt="Interní programovací jazyk" align="middle" hspace="2"/>
</xsl:when>
<xsl:when test="self::node()[@lang='sql']">
<img src="../../images/sql.gif" width="44"
	 height="16" border="0" alt="SQL" align="middle" hspace="2"/>
</xsl:when>
<xsl:when test="self::node()[@lang='pas']">
<img src="../../images/pascal.gif" width="44"
	 height="16" border="0" alt="Pascal" align="middle" hspace="2"/>
</xsl:when>
<xsl:when test="self::node()[@lang='c']">
<img src="../../images/cpp.gif" width="44"
	 height="16" border="0" alt="C/C++" align="middle" hspace="2"/>
</xsl:when>
<xsl:when test="self::node()[@lang='php']">
<img src="../../images/php.gif" width="44"
	 height="16" border="0" alt="PHP" align="middle" hspace="2"/>
</xsl:when>
</xsl:choose>
</xsl:for-each>
<br/>
<br/>
<div class='typ'><xsl:for-each select="decl"><xsl:apply-templates/>
<br/>
</xsl:for-each></div>
<xsl:apply-templates/>
</xsl:template>

<xsl:template match="params[../../function-desc]"><br/><br/><h3>Parametry</h3><xsl:apply-templates/></xsl:template>
<xsl:template match="version[../../function-desc]"><br/><br/><h3>Od verze</h3><xsl:apply-templates/></xsl:template>
<xsl:template match="returns[../../function-desc]"><br/><br/><h3>Návratová hodnota</h3><xsl:apply-templates/></xsl:template>
<xsl:template match="desc"><br/><br/><h3>Popis</h3><xsl:apply-templates/></xsl:template>
<xsl:template match="example"><br/><br/><h3>Pøíklad</h3><xsl:apply-templates/></xsl:template>


<xsl:template match="decl">
</xsl:template>


<!-- Images -->
<xsl:template match="img">
<img>
<xsl:attribute name="width">
<xsl:value-of select="@width"/>
</xsl:attribute>
<xsl:attribute name="height">
<xsl:value-of select="@height"/>
</xsl:attribute>
<xsl:attribute name="src">
<xsl:value-of select="@src"/>
</xsl:attribute>
<xsl:attribute name="alt">
<xsl:value-of select="@alt"/>
</xsl:attribute>
<xsl:attribute name="border">
<xsl:value-of select="@border"/>
</xsl:attribute>
</img>
</xsl:template>



<xsl:template match="see">
<h3>Viz</h3>
<ul>
<xsl:for-each select="a">
<li><img src="../../images/viz.gif"/>
<xsl:apply-templates select="."/>
</li>
</xsl:for-each>
</ul>
</xsl:template>

<xsl:template match="a[@xmlrefname]">
<a>
<xsl:attribute name="href"><xsl:value-of select="substring-before(@xmlrefname,'#')"/>.html#<xsl:value-of select="substring-after(@xmlrefname,'#')"/></xsl:attribute>
<xsl:apply-templates/></a>
</xsl:template>

<xsl:template match="a[@xmlref]">
<a>
<xsl:attribute name="href"><xsl:value-of select="@xmlref"/>.html</xsl:attribute>
<xsl:apply-templates/></a>
</xsl:template>

<xsl:template match="a[@href]">
<a>
<xsl:attribute name="href">
<xsl:value-of select="@href"/>
</xsl:attribute>
<xsl:apply-templates/></a>
</xsl:template>

<xsl:template match="a[@name]">
<a>
<xsl:attribute name="name">
<xsl:value-of select="@name"/>
</xsl:attribute>
<xsl:apply-templates/></a>
</xsl:template>

<xsl:template match="a[@file]">
<a>
<xsl:attribute name="href"><xsl:value-of select="@file"/></xsl:attribute>
<xsl:value-of select="@file"/></a>
</xsl:template>


<xsl:template match="dl"><dl><xsl:apply-templates/></dl></xsl:template>
<xsl:template match="dd"><dd><xsl:apply-templates/></dd></xsl:template>
<xsl:template match="ol"><ol>
<xsl:attribute name="start"><xsl:value-of select="@start"/></xsl:attribute>
<xsl:apply-templates/></ol></xsl:template>
<xsl:template match="ul"><ul><xsl:apply-templates/></ul></xsl:template>
<xsl:template match="li"><li><xsl:apply-templates/></li></xsl:template>
<xsl:template match="tr"><tr><xsl:apply-templates/></tr></xsl:template>
<xsl:template match="td"><td><xsl:apply-templates/></td></xsl:template>
<xsl:template match="p"><p><xsl:apply-templates/></p></xsl:template>
<xsl:template match="br"><br><xsl:apply-templates/></br></xsl:template>

<xsl:template match="table">
<table cellspacing="1" cellpadding="2">
	<xsl:attribute name='border'><xsl:value-of select="@border"/></xsl:attribute>
<xsl:apply-templates/>
</table>
</xsl:template>

<xsl:template match="dt"><dt><dfn><xsl:apply-templates/></dfn></dt></xsl:template>
<xsl:template match="em"><em><xsl:value-of select="."/></em></xsl:template>
<xsl:template match="code|verbatim"><pre><xsl:attribute name='class'><xsl:value-of select='@style'/></xsl:attribute><xsl:apply-templates/></pre></xsl:template>
<xsl:template match="tt|file|ident"><tt><xsl:apply-templates/></tt></xsl:template>

<!-- Znacky pro typy textu -->
<xsl:template match="menuitem"><strong class="menuitem"><xsl:apply-templates/></strong></xsl:template>
<xsl:template match="button"><strong class="button"><xsl:apply-templates/></strong></xsl:template>
<xsl:template match="program"><span class="program"><xsl:apply-templates/></span></xsl:template>
<xsl:template match="dfn"><dfn><xsl:apply-templates/></dfn></xsl:template>

<xsl:template match="strong"><strong><xsl:apply-templates/></strong></xsl:template>

<!-- Structure of resulting HTML document -->
<xsl:template match="/">
<html>
<head>
<meta http-equiv="Content-Type" Content="text/html; charset=windows-1250"/>
<link rel="stylesheet" href="docstyle.css"/>
<title><xsl:value-of select="Section-1/Header|function-desc/@name|CDK-desc/@name"/></title>
</head>
<body>
<xsl:apply-templates/>
</body>
</html>
</xsl:template>


</xsl:stylesheet>
<!-- vim: set syntax=xml: -->
