/////////////////////////////////////////////////////////////////////////////
// Name:        AboutOpenDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/18/07 11:02:12
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "AboutOpenDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#include "wx/hyperlink.h"
extern char *logo_xpm[];

////@begin includes
////@end includes

#include "AboutOpenDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * AboutOpenDlg type definition
 */

IMPLEMENT_DYNAMIC_CLASS( AboutOpenDlg, wxDialog )

/*!
 * AboutOpenDlg event table definition
 */

BEGIN_EVENT_TABLE( AboutOpenDlg, wxDialog )

////@begin AboutOpenDlg event table entries
////@end AboutOpenDlg event table entries

END_EVENT_TABLE()

/*!
 * AboutOpenDlg constructors
 */

AboutOpenDlg::AboutOpenDlg( )
{
}

AboutOpenDlg::AboutOpenDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * AboutOpenDlg creator
 */

bool AboutOpenDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin AboutOpenDlg member initialisation
    mBitmap = NULL;
    mSizer = NULL;
    mMainName = NULL;
    mBuild = NULL;
    mCopyr1 = NULL;
    mCopyr2 = NULL;
    mURL = NULL;
    mLicenc = NULL;
////@end AboutOpenDlg member initialisation

////@begin AboutOpenDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end AboutOpenDlg creation
    return TRUE;
}

/*!
 * Control creation for AboutOpenDlg
 */
/*
char * lic_en = "GNU LESSER GENERAL PUBLIC LICENSE\n"
                       "Version 3, 29 June 2007\n\n"
 "Copyright (C) 2007 Free Software Foundation, Inc. <http://fsf.org/>\n\n"
 "Everyone is permitted to copy and distribute verbatim copies of this license document, but changing it is not allowed.\n\n"
  "This version of the GNU Lesser General Public License incorporates the terms and conditions of version 3 of the GNU General Public License, supplemented by the additional permissions listed below.\n\n"
  "0. Additional Definitions. \n\n"
  "As used herein, \"this License\" refers to version 3 of the GNU Lesser General Public License, and the \"GNU GPL\" refers to version 3 of the GNU General Public License.\n\n"
  "\"The Library\" refers to a covered work governed by this License, other than an Application or a Combined Work as defined below.\n\n"
  "An \"Application\" is any work that makes use of an interface provided by the Library, but which is not otherwise based on the Library. Defining a subclass of a class defined by the Library is deemed a mode of using an interface provided by the Library.\n\n"
  "A \"Combined Work\" is a work produced by combining or linking an Application with the Library.  The particular version of the Library with which the Combined Work was made is also called the \"Linked Version\".\n\n"
  "The \"Minimal Corresponding Source\" for a Combined Work means the Corresponding Source for the Combined Work, excluding any source code for portions of the Combined Work that, considered in isolation, are based on the Application, and not on the Linked Version.\n\n"
  "The \"Corresponding Application Code\" for a Combined Work means the object code and/or source code for the Application, including any data and utility programs needed for reproducing the Combined Work from the Application, but excluding the System Libraries of the Combined Work.\n\n"
  "1. Exception to Section 3 of the GNU GPL.\n\n"
  "You may convey a covered work under sections 3 and 4 of this License without being bound by section 3 of the GNU GPL.\n\n"
  "2. Conveying Modified Versions.\n\n"
  "If you modify a copy of the Library, and, in your modifications, a facility refers to a function or data to be supplied by an Application that uses the facility (other than as an argument passed when the facility is invoked), then you may convey a copy of the modified version:\n\n"
   "a) under this License, provided that you make a good faith effort to ensure that, in the event an Application does not supply the function or data, the facility still operates, and performs whatever part of its purpose remains meaningful, or\n\n"
   "b) under the GNU GPL, with none of the additional permissions of this License applicable to that copy.\n\n"
  "3. Object Code Incorporating Material from Library Header Files.\n\n"
  "The object code form of an Application may incorporate material from a header file that is part of the Library. You may convey such object code under terms of your choice, provided that, if the incorporated material is not limited to numerical parameters, data structure layouts and accessors, or small macros, inline functions and templates (ten or fewer lines in length), you do both of the following:\n\n"
   "a) Give prominent notice with each copy of the object code that the Library is used in it and that the Library and its use are covered by this License.\n\n"
   "b) Accompany the object code with a copy of the GNU GPL and this license document.\n\n"
  "4. Combined Works.\n\n"
  "You may convey a Combined Work under terms of your choice that, taken together, effectively do not restrict modification of the portions of the Library contained in the Combined Work and reverse engineering for debugging such modifications, if you also do each of the following:\n\n"
   "a) Give prominent notice with each copy of the Combined Work that the Library is used in it and that the Library and its use are covered by this License.\n\n"
   "b) Accompany the Combined Work with a copy of the GNU GPL and this license document.\n\n"
   "c) For a Combined Work that displays copyright notices during execution, include the copyright notice for the Library among these notices, as well as a reference directing the user to the copies of the GNU GPL and this license document.\n\n"
   "d) Do one of the following:\n\n"
       "0) Convey the Minimal Corresponding Source under the terms of this License, and the Corresponding Application Code in a form suitable for, and under terms that permit, the user to recombine or relink the Application with a modified version of the Linked Version to produce a modified Combined Work, in the manner specified by section 6 of the GNU GPL for conveying Corresponding Source.\n\n"
       "1) Use a suitable shared library mechanism for linking with the Library.  A suitable mechanism is one that (a) uses at run time a copy of the Library already present on the user's computer system, and (b) will operate properly with a modified version of the Library that is interface-compatible with the Linked Version. \n\n"
   "e) Provide Installation Information, but only if you would otherwise be required to provide such information under section 6 of the GNU GPL, and only to the extent that such information is necessary to install and execute a modified version of the Combined Work produced by recombining or relinking the Application with a modified version of the Linked Version. (If you use option 4d0, the Installation Information must accompany the Minimal Corresponding Source and Corresponding Application Code. If you use option 4d1, you must provide the Installation Information in the manner specified by section 6 of the GNU GPL for conveying Corresponding Source.)\n\n"
  "5. Combined Libraries.\n\n"
  "You may place library facilities that are a work based on the Library side by side in a single library together with other library facilities that are not Applications and are not covered by this License, and convey such a combined library under terms of your choice, if you do both of the following:\n\n"
   "a) Accompany the combined library with a copy of the same work based on the Library, uncombined with any other library facilities, conveyed under the terms of this License.\n\n"
   "b) Give prominent notice with the combined library that part of it is a work based on the Library, and explaining where to find the accompanying uncombined form of the same work.\n\n"
  "6. Revised Versions of the GNU Lesser General Public License.\n\n"
  "The Free Software Foundation may publish revised and/or new versions of the GNU Lesser General Public License from time to time. Such new versions will be similar in spirit to the present version, but may differ in detail to address new problems or concerns.\n\n"
  "Each version is given a distinguishing version number. If the Library as you received it specifies that a certain numbered version of the GNU Lesser General Public License \"or any later version\" applies to it, you have the option of following the terms and conditions either of that published version or of any later version published by the Free Software Foundation. If the Library as you received it does not specify a version number of the GNU Lesser General Public License, you may choose any version of the GNU Lesser General Public License ever published by the Free Software Foundation.\n\n"
  "If the Library as you received it specifies that a proxy can decide whether future versions of the GNU Lesser General Public License shall apply, that proxy's public statement of acceptance of any version is permanent authorization for you to choose that version for the Library.\n\n";
*/

char * lic_en = "GNU LESSER GENERAL PUBLIC LICENSE\n"
" Version 2.1, February 1999\n\n"

" Copyright (C) 1991, 1999 Free Software Foundation, Inc. 51 Franklin Street, Fifth Floor, Boston, MA02110-1301USA\n"
" Everyone is permitted to copy and distribute verbatim copies of this license document, but changing it is not allowed.\n\n"

"[This is the first released version of the Lesser GPL.It also counts as the successor of the GNU Library Public License, version 2, hence the version number 2.1.]\n\n"

"Preamble\n\n"

"The licenses for most software are designed to take away your freedom to share and change it.By contrast, the GNU General Public Licenses are intended to guarantee your freedom to share and change free software--to make sure the software is free for all its users.\n\n"

"This license, the Lesser General Public License, applies to some specially designated software packages--typically libraries--of the Free Software Foundation and other authors who decide to use it.You can use it too, but we suggest you first think carefully about whether this license or the ordinary General Public License is the better strategy to use in any particular case, based on the explanations below.\n\n"

"When we speak of free software, we are referring to freedom of use, not price.Our General Public Licenses are designed to make sure that you have the freedom to distribute copies of free software (and charge for this service if you wish); that you receive source code or can get it if you want it; that you can change the software and use pieces of it in new free programs; and that you are informed that you can do these things.\n\n"

"To protect your rights, we need to make restrictions that forbid distributors to deny you these rights or to ask you to surrender these rights.These restrictions translate to certain responsibilities for you if you distribute copies of the library or if you modify it.\n\n"

"For example, if you distribute copies of the library, whether gratis or for a fee, you must give the recipients all the rights that we gave you.You must make sure that they, too, receive or can get the source code.If you link other code with the library, you must provide complete object files to the recipients, so that they can relink them with the library after making changes to the library and recompiling it.And you must show them these terms so they know their rights.\n\n"

"We protect your rights with a two-step method: (1) we copyright the library, and (2) we offer you this license, which gives you legal permission to copy, distribute and/or modify the library.\n\n"

"To protect each distributor, we want to make it very clear that there is no warranty for the free library.Also, if the library is modified by someone else and passed on, the recipients should know that what they have is not the original version, so that the original author's reputation will not be affected by problems that might be introduced by others.\n\n"

"Finally, software patents pose a constant threat to the existence of any free program.We wish to make sure that a company cannot effectively restrict the users of a free program by obtaining a restrictive license from a patent holder.Therefore, we insist that any patent license obtained for a version of the library must be consistent with the full freedom of use specified in this license.\n\n"

"Most GNU software, including some libraries, is covered by the ordinary GNU General Public License.This license, the GNU Lesser General Public License, applies to certain designated libraries, and is quite different from the ordinary General Public License.We use this license for certain libraries in order to permit linking those libraries into non-free programs.\n\n"

"When a program is linked with a library, whether statically or using a shared library, the combination of the two is legally speaking a combined work, a derivative of the original library.The ordinary General Public License therefore permits such linking only if the entire combination fits its criteria of freedom.The Lesser General Public License permits more lax criteria for linking other code with the library.\n\n"

"We call this license the \"Lesser\" General Public License because it does Less to protect the user's freedom than the ordinary General Public License.It also provides other free software developers Less of an advantage over competing non-free programs.These disadvantages are the reason we use the ordinary General Public License for many libraries.However, the Lesser license provides advantages in certain special circumstances.\n\n"

"For example, on rare occasions, there may be a special need to encourage the widest possible use of a certain library, so that it becomes a de-facto standard.To achieve this, non-free programs must be allowed to use the library.A more frequent case is that a free library does the same job as widely used non-free libraries.In this case, there is little to gain by limiting the free library to free software only, so we use the Lesser General Public License.\n\n"

"In other cases, permission to use a particular library in non-free programs enables a greater number of people to use a large body of free software.For example, permission to use the GNU C Library in non-free programs enables many more people to use the whole GNU operating system, as well as its variant, the GNU/Linux operating system.\n\n"

"Although the Lesser General Public License is Less protective of the users' freedom, it does ensure that the user of a program that is linked with the Library has the freedom and the wherewithal to run that program using a modified version of the Library.\n\n"

"The precise terms and conditions for copying, distribution and modification follow.Pay close attention to the difference between a \"work based on the library\" and a \"work that uses the library\".The former contains code derived from the library, whereas the latter must be combined with the library in order to run.\n\n"

"		GNU LESSER GENERAL PUBLIC LICENSE\n"
" TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION\n\n"

"0. This License Agreement applies to any software library or other program which contains a notice placed by the copyright holder or other authorized party saying it may be distributed under the terms of this Lesser General Public License (also called \"this License\"). Each licensee is addressed as \"you\".\n\n"

"A \"library\" means a collection of software functions and/or data prepared so as to be conveniently linked with application programs (which use some of those functions and data) to form executables.\n\n"

"The \"Library\", below, refers to any such software library or work which has been distributed under these terms.A \"work based on the Library\" means either the Library or any derivative work under copyright law: that is to say, a work containing the Library or a portion of it, either verbatim or with modifications and/or translated straightforwardly into another language.(Hereinafter, translation is included without limitation in the term \"modification\".)\n\n"

"\"Source code\" for a work means the preferred form of the work for making modifications to it.For a library, complete source code means all the source code for all modules it contains, plus any associated interface definition files, plus the scripts used to control compilation and installation of the library.\n\n"

"Activities other than copying, distribution and modification are not covered by this License; they are outside its scope.The act of running a program using the Library is not restricted, and output from such a program is covered only if its contents constitute a work based on the Library (independent of the use of the Library in a tool for writing it).Whether that is true depends on what the Library does and what the program that uses the Library does.\n\n"

"1. You may copy and distribute verbatim copies of the Library's complete source code as you receive it, in any medium, provided that you conspicuously and appropriately publish on each copy an appropriate copyright notice and disclaimer of warranty; keep intact all the notices that refer to this License and to the absence of any warranty; and distribute a copy of this License along with the Library.\n\n"

"You may charge a fee for the physical act of transferring a copy, and you may at your option offer warranty protection in exchange for a fee.\n\n"

"2. You may modify your copy or copies of the Library or any portion of it, thus forming a work based on the Library, and copy and distribute such modifications or work under the terms of Section 1 above, provided that you also meet all of these conditions:\n\n"

"a) The modified work must itself be a software library.\n"
"b) You must cause the files modified to carry prominent notices stating that you changed the files and the date of any change.\n"
"c) You must cause the whole of the work to be licensed at no charge to all third parties under the terms of this License.\n"
"d) If a facility in the modified Library refers to a function or a table of data to be supplied by an application program that uses the facility, other than as an argument passed when the facility is invoked, then you must make a good faith effort to ensure that, in the event an application does not supply such function or table, the facility still operates, and performs whatever part of its purpose remains meaningful. (For example, a function in a library to compute square roots has a purpose that is entirely well-defined independent of the application.Therefore, Subsection 2d requires that any application-supplied function or table used by this function must be optional: if the application does not supply it, the square root function must still compute square roots.)\n\n"

"These requirements apply to the modified work as a whole.If identifiable sections of that work are not derived from the Library, and can be reasonably considered independent and separate works in themselves, then this License, and its terms, do not apply to those sections when you distribute them as separate works.But when you distribute the same sections as part of a whole which is a work based on the Library, the distribution of the whole must be on the terms of this License, whose permissions for other licensees extend to the entire whole, and thus to each and every part regardless of who wrote it.\n\n"

"Thus, it is not the intent of this section to claim rights or contest your rights to work written entirely by you; rather, the intent is to exercise the right to control the distribution of derivative or collective works based on the Library.\n\n"

"In addition, mere aggregation of another work not based on the Library with the Library (or with a work based on the Library) on a volume of a storage or distribution medium does not bring the other work under the scope of this License.\n\n"

"3. You may opt to apply the terms of the ordinary GNU General Public License instead of this License to a given copy of the Library.To do this, you must alter all the notices that refer to this License, so that they refer to the ordinary GNU General Public License, version 2, instead of to this License.(If a newer version than version 2 of the ordinary GNU General Public License has appeared, then you can specify that version instead if you wish.)Do not make any other change in these notices.\n\n"

"Once this change is made in a given copy, it is irreversible for that copy, so the ordinary GNU General Public License applies to all subsequent copies and derivative works made from that copy.\n\n"

"This option is useful when you wish to copy part of the code of the Library into a program that is not a library.\n\n"

"4. You may copy and distribute the Library (or a portion or derivative of it, under Section 2) in object code or executable form under the terms of Sections 1 and 2 above provided that you accompany it with the complete corresponding machine-readable source code, which must be distributed under the terms of Sections 1 and 2 above on a medium customarily used for software interchange.\n\n"

"If distribution of object code is made by offering access to copy from a designated place, then offering equivalent access to copy the source code from the same place satisfies the requirement to distribute the source code, even though third parties are not compelled to copy the source along with the object code.\n\n"

"5. A program that contains no derivative of any portion of the Library, but is designed to work with the Library by being compiled or linked with it, is called a \"work that uses the Library\".Such a work, in isolation, is not a derivative work of the Library, and therefore falls outside the scope of this License.\n\n"

"However, linking a \"work that uses the Library\" with the Library creates an executable that is a derivative of the Library (because it contains portions of the Library), rather than a \"work that uses the library\".The executable is therefore covered by this License. Section 6 states terms for distribution of such executables.\n\n"

"When a \"work that uses the Library\" uses material from a header file that is part of the Library, the object code for the work may be a derivative work of the Library even though the source code is not. Whether this is true is especially significant if the work can be linked without the Library, or if the work is itself a library.The threshold for this to be true is not precisely defined by law.\n\n"

"If such an object file uses only numerical parameters, data structure layouts and accessors, and small macros and small inline functions (ten lines or less in length), then the use of the object file is unrestricted, regardless of whether it is legally a derivative work.(Executables containing this object code plus portions of the Library will still fall under Section 6.)\n\n"

"Otherwise, if the work is a derivative of the Library, you may distribute the object code for the work under the terms of Section 6. Any executables containing that work also fall under Section 6, whether or not they are linked directly with the Library itself.\n\n" 

"6. As an exception to the Sections above, you may also combine or link a \"work that uses the Library\" with the Library to produce a work containing portions of the Library, and distribute that work under terms of your choice, provided that the terms permit modification of the work for the customer's own use and reverseengineering for debugging such modifications.\n\n"

"You must give prominent notice with each copy of the work that the Library is used in it and that the Library and its use are covered by this License.You must supply a copy of this License.If the work during execution displays copyright notices, you must include the copyright notice for the Library among them, as well as a reference directing the user to the copy of this License.Also, you must do one of these things:\n\n"

"a) Accompany the work with the complete corresponding machine-readable source code for the Library including whatever changes were used in the work (which must be distributed under Sections 1 and 2 above); and, if the work is an executable linked with the Library, with the complete machine-readable \"work that uses the Library\", as object code and/or source code, so that the user can modify the Library and then relink to produce a modified executable containing the modified Library.(It is understood that the user who changes the contents of definitions files in the Library will not necessarily be able to recompile the application to use the modified definitions.)\n"
"b) Use a suitable shared library mechanism for linking with the Library.A suitable mechanism is one that (1) uses at run time a copy of the library already present on the user's computer system, rather than copying library functions into the executable, and (2) will operate properly with a modified version of the library, if the user installs one, as long as the modified version is interface-compatible with the version that the work was made with.\n"
"c) Accompany the work with a written offer, valid for at least three years, to give the same user the materials specified in Subsection 6a, above, for a charge no more than the cost of performing this distribution.\n"
"d) If distribution of the work is made by offering access to copy from a designated place, offer equivalent access to copy the above specified materials from the same place.\n"
"e) Verify that the user has already received a copy of these materials or that you have already sent this user a copy.\n\n"

"For an executable, the required form of the \"work that uses the Library\" must include any data and utility programs needed for reproducing the executable from it.However, as a special exception, the materials to be distributed need not include anything that is normally distributed (in either source or binary form) with the majorcomponents (compiler, kernel, and so on) of the operating system on which the executable runs, unless that component itself accompanies the executable.\n\n"

"It may happen that this requirement contradicts the license restrictions of other proprietary libraries that do not normally accompany the operating system.Such a contradiction means you cannot use both them and the Library together in an executable that you distribute.\n\n"

"7. You may place library facilities that are a work based on the Library side-by-side in a single library together with other library facilities not covered by this License, and distribute such a combined library, provided that the separate distribution of the work based on the Library and of the other library facilities is otherwise permitted, and provided that you do these two things:\n\n"

"a) Accompany the combined library with a copy of the same work based on the Library, uncombined with any other library facilities.This must be distributed under the terms of the Sections above.\n\n"

"b) Give prominent notice with the combined library of the fact that part of it is a work based on the Library, and explaining where to find the accompanying uncombined form of the same work.\n\n"

"8. You may not copy, modify, sublicense, link with, or distribute the Library except as expressly provided under this License.Any attempt otherwise to copy, modify, sublicense, link with, or distribute the Library is void, and will automatically terminate your rights under this License.However, parties who have received copies, or rights, from you under this License will not have their licenses terminated so long as such parties remain in full compliance.\n\n"

"9. You are not required to accept this License, since you have not signed it.However, nothing else grants you permission to modify or distribute the Library or its derivative works.These actions are prohibited by law if you do not accept this License.Therefore, by modifying or distributing the Library (or any work based on the Library), you indicate your acceptance of this License to do so, and all its terms and conditions for copying, distributing or modifying the Library or works based on it.\n\n"

"10. Each time you redistribute the Library (or any work based on the Library), the recipient automatically receives a license from the original licensor to copy, distribute, link with or modify the Library subject to these terms and conditions.You may not impose any further restrictions on the recipients' exercise of the rights granted herein. You are not responsible for enforcing compliance by third parties with this License.\n\n"

"11. If, as a consequence of a court judgment or allegation of patent infringement or for any other reason (not limited to patent issues), conditions are imposed on you (whether by court order, agreement or otherwise) that contradict the conditions of this License, they do not excuse you from the conditions of this License.If you cannot distribute so as to satisfy simultaneously your obligations under this License and any other pertinent obligations, then as a consequence you may not distribute the Library at all.For example, if a patent license would not permit royalty-free redistribution of the Library by all those who receive copies directly or indirectly through you, then the only way you could satisfy both it and this License would be to refrain entirely from distribution of the Library.\n\n"

"If any portion of this section is held invalid or unenforceable under any particular circumstance, the balance of the section is intended to apply, and the section as a whole is intended to apply in other circumstances.\n\n"

"It is not the purpose of this section to induce you to infringe any patents or other property right claims or to contest validity of any such claims; this section has the sole purpose of protecting the integrity of the free software distribution system which is implemented by public license practices.Many people have made generous contributions to the wide range of software distributed through that system in reliance on consistent application of that system; it is up to the author/donor to decide if he or she is willing to distribute software through any other system and a licensee cannot impose that choice.\n\n"

"This section is intended to make thoroughly clear what is believed to be a consequence of the rest of this License.\n\n"

"12. If the distribution and/or use of the Library is restricted in certain countries either by patents or by copyrighted interfaces, the original copyright holder who places the Library under this License may add an explicit geographical distribution limitation excluding those countries, so that distribution is permitted only in or among countries not thus excluded.In such case, this License incorporates the limitation as if written in the body of this License.\n\n"

"13. The Free Software Foundation may publish revised and/or new versions of the Lesser General Public License from time to time. Such new versions will be similar in spirit to the present version, but may differ in detail to address new problems or concerns.\n\n"

"Each version is given a distinguishing version number.If the Library specifies a version number of this License which applies to it and \"any later version\", you have the option of following the terms and conditions either of that version or of any later version published by the Free Software Foundation.If the Library does not specify a license version number, you may choose any version ever published by the Free Software Foundation.\n\n"

"14. If you wish to incorporate parts of the Library into other free programs whose distribution conditions are incompatible with these, write to the author to ask for permission.For software which is copyrighted by the Free Software Foundation, write to the Free Software Foundation; we sometimes make exceptions for this.Our decision will be guided by the two goals of preserving the free status of all derivatives of our free software and of promoting the sharing and reuse of software generally.\n\n"

"NO WARRANTY\n\n"

"15. BECAUSE THE LIBRARY IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY FOR THE LIBRARY, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE LIBRARY \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE LIBRARY IS WITH YOU.SHOULD THE LIBRARY PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.\n\n"

"16. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR REDISTRIBUTE THE LIBRARY AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE LIBRARY (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE LIBRARY TO OPERATE WITH ANY OTHER SOFTWARE), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.\n\n";

char * lic_cs = 
"GNU Lesser General Public License\n"
"Český překlad verze 2.1, únor 1999\n\n"

"    Copyright (C) 1991, 1999 Free Software Foundation, Inc. 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n"
"    Každému je povoleno kopírovat a šířit doslovné kopie tohoto licenčního dokumentu, ale není dovoleno jej měnit.\n\n"

"    [Toto je první vydaná verze Lesser GPL. Také se počítá jako následník GNU Library Public License, verze 2, odtud číslo verze 2.1.]\n\n"

"Úvod\n\n"

"Licence pro většinu softwaru jsou navrženy tak, aby vám sebraly svobodu sdílet ho a měnit. Naproti tomu GNU General Public Licenses jsou určeny k tomu, aby vám svobodu sdílet a měnit svobodný software zaručovaly – s cílem zajistit, že software bude svobodný pro všechny jeho uživatele.\n\n"

"Tato licence, Lesser General Public License, se vztahuje na některé softwarové balíčky se zvláštním určením – typicky knihovny – od Free Software Foundation a dalších autorů, kteří se rozhodnou ji použít. Použít ji můžete i vy, ale doporučujeme vám, abyste si na základě dalšího výkladu nejprve důkladně rozmysleli, zda je v tom kterém případě lepší strategií použít tuto licenci, anebo běžnou General Public License.\n\n"

"Mluvíme-li o svobodném softwaru, máme na mysli svobodu používání, nikoli cenu. Naše General Public Licenses jsou navrženy tak, aby zajišťovaly, že budete mít svobodu šířit kopie svobodného softwaru (a tuto službu si zpoplatnit, budete-li chtít); že dostanete zdrojový kód nebo budete mít možnost si ho pořídit; že můžete daný software měnit a používat jeho části v nových svobodných programech; a že vám bude oznámeno, že tyto věci můžete dělat.\n\n"

"Abychom mohli ochránit vaše práva, musíme vytvořit omezení, která znemožní distributorům vám tato práva upírat nebo žádat po vás, abyste se těchto práv vzdali. Pokud šíříte kopie knihovny nebo ji upravujete, mění se pro vás tato omezení v určité závazky.\n\n"

"Například, jestliže šíříte kopie knihovny, ať už gratis nebo za peníze, musíte příjemcům poskytnout všechna práva, která my poskytujeme vám. Musíte zajistit, aby i oni dostali nebo měli možnost pořídit si zdrojový kód. Pokud s knihovnou sestavujete další kód, musíte dát příjemcům k dispozici kompletní objektové soubory, aby poté, co provedou v knihovně změny a znovu ji zkompilují, mohli tyto objektové soubory s knihovnou opět sestavit. A musíte jim ukázat tyto podmínky, aby znali svá práva.\n\n"

"Vaše práva chráníme metodou sestávající ze dvou kroků: (1) Opatříme knihovnu copyrightem a (2) nabídneme vám tuto licenci, která vám poskytuje zákonné povolení knihovnu kopírovat, šířit a/nebo upravovat.\n\n"

"Abychom ochránili každého distributora, chceme zajistit, aby bylo velmi jasné, že na svobodné knihovny se nevztahuje žádná záruka. Také pokud knihovnu upraví někdo jiný a předá ji dál, měli by příjemci vědět, že to, co mají, není původní verze, aby problémy, které by mohli způsobit druzí, nekazily pověst původního autora.\n\n"

"Nakonec, neustálou hrozbu pro existenci jakéhokoli svobodného programu představují softwarové patenty. Přejeme si zajistit, aby žádná firma nemohla účinně omezovat uživatele svobodného programu tím, že od držitele patentu získá restriktivní licenci. A proto trváme na tom, že jakákoli patentová licence získaná pro určitou verzi knihovny musí být v souladu s úplnou svobodou používání, specifikovanou v této licenci.\n\n"

"Na většinu softwaru GNU, včetně některých knihoven, se vztahuje obyčejná GNU General Public License. Tato licence, GNU Lesser General Public License, se vztahuje na některé určené knihovny a je úplně jiná než obyčejná General Public License. Tuto licenci použijeme pro určité knihovny, abychom je povolili sestavovat s nesvobodnými programy.\n\n"

"Když se nějaký program sestaví s knihovnou, ať už staticky nebo za použití sdílené knihovny, je kombinace těchto dvou z právního hlediska kombinovaným dílem, odvozeninou původní knihovny. Obyčejná General Public License proto dovoluje takové sestavení pouze tehdy, vyhoví-li celá kombinace jejím měřítkům svobody. Lesser General Public License stanovuje pro sestavování jiného kódu s knihovnou volnější měřítka.\n\n"

"Této licenci říkáme \"Lesser\" (menší) General Public License proto, že pro ochranu svobody uživatele dělá méně (less) než obyčejná GNU General Public License. Také vývojářům dalšího svobodného softwaru poskytuje méně výhod oproti konkurenčním nesvobodným programům. Tyto nevýhody jsou důvodem, proč pro mnoho knihoven používáme obyčejnou GNU General Public License. Nicméně za určitých zvláštních okolností tato menší licence přináší výhody.\n\n"

"Například, ač zřídka, se může vyskytnout zvláštní potřeba podpořit co nejmasovější používání určité knihovny, aby se tato stala de facto standardem. K dosažení toho je nutné umožnit využití knihovny i pro nesvobodné programy. Častějším případem je, že určitá svobodná knihovna má stejnou funkci, jako běžněji používané nesvobodné knihovny. V tomto případě se omezením svobodné knihovny výhradně pro svobodné programy dá získat jen málo, proto použijeme Lesser General Public License.\n\n"

"V jiných případech povolení používat určitou knihovnu v nesvobodných programech umožní používat spoustu svobodného softwaru většímu počtu lidí. Například, povolení používat knihovnu GNU C v nesvobodných programech umožňuje mnohem více lidem používat celý operační systém GNU a rovněž tak jeho variantu, operační systém GNU/Linux.\n\n"

"Ačkoli Lesser General Public License chrání svobodu uživatelů méně, zajišťuje, že uživatel programu, který byl sestaven s danou knihovnou, bude mít svobodu a finanční prostředky potřebné pro provozování tohoto programu i při používání modifikované verze knihovny.\n\n"

"Dále uvádíme přesné podmínky pro kopírování, šíření a úpravy. Dávejte si dobrý pozor na rozdíl mezi \"dílem založeným na knihovně\" a \"dílem, které používá knihovnu\". První obsahuje kód odvozený z knihovny, druhé se musí použít s knihovnou, aby mohlo běžet.\n\n"

"PODMÍNKY PRO KOPÍROVÁNÍ, ŠÍŘENÍ A ÚPRAVY\n\n"

"0. Tato licenční smlouva se vztahuje na jakoukoli softwarovou knihovnu nebo jiný program obsahující vyrozumění vložené držitelem autorských práv nebo jinou oprávněnou stranou, kde stojí, že daný software smí být šířen v souladu s podmínkami této Lesser General Public License (jinými slovy \"této licence\"). Každý držitel licence je oslovován \"vy\".\n\n"

"\"Knihovna\" znamená sbírku softwarových funkcí a/nebo dat připravených tak, aby mohly být vhodně sestavovány s aplikačními programy (jež některé tyto funkce a data využívají) za účelem vytvoření spustitelných souborů.\n\n"

"Dále v textu se \"Knihovna\" týká jakékoli knihovny nebo díla, které jsou šířeny za těchto podmínek. \"Dílo založené na Knihovně\" znamená Knihovna, nebo jakékoli dílo odvozené pod ochranou autorského zákona: jinými slovy dílo obsahující Knihovnu nebo její část beze změn, nebo s úpravami a/nebo přímo přeloženou do jiného jazyka. (Odtud dál se překlad zahrnuje bez omezení do termínu \"úprava\".)\n\n"

"\"Zdrojový kód\" k dílu znamená podoba díla upřednostňovaná pro provádění úprav na něm. U knihovny úplný zdrojový kód znamená všechen zdrojový kód pro všechny moduly, které knihovna obsahuje, plus jakékoli přidružené soubory s definicemi rozhraní plus skripty používané pro řízení kompilace a instalace knihovny.\n\n"

"Na činnosti jiné než kopírování, šíření a upravování se tato licence nevztahuje; jsou mimo její rozsah platnosti. Na provozování programu využívajícího Knihovnu se nekladou žádná omezení a výstupu takového programu se licence týká jen v případě, že představuje dílo založené na Knihovně (nezávisle na použití Knihovny nástrojem pro jeho psaní). Zda je tomu tak, záleží na tom, co dělá Knihovna a co dělá program, který Knihovnu využívá.\n\n"

"1. Doslovné kopie zdrojového kódu knihovny smíte kopírovat a šířit tak, jak je dostanete, na jakékoli médiu za předpokladu, že na každé kopii viditelně a vhodně zveřejníte vyrozumění o autorských právech a popření záruky; necháte beze změny všechna vyrozumění související s touto licencí a absencí jakékoli záruky; a spolu s Knihovnou budete šířit kopii této licence.\n\n"

"Za kopírování jako fyzický úkon si smíte účtovat poplatek a dle svého uvážení smíte nabízet výměnou za poplatek záruční ochranu.\n\n"

"2. Svou kopii nebo kopie Knihovny nebo jakékoli její části smíte upravovat, tedy utvářet dílo založené na Knihovně, a tyto úpravy nebo dílo kopírovat a šířit v souladu s podmínkami z oddílu 1 výše za předpokladu, že také vyhovíte všem těmto podmínkám:\n\n"

"      a) Upravené dílo samo musí být softwarová knihovna.\n"
"      b) Musíte zajistit, aby všechny upravené soubory obsahovaly dobře viditelná vyrozumění, ve kterých bude stát, že vy jste soubory změnili a kdy se tak stalo.\n"
"      c) Musíte zajistit, aby všem třetím stranám byla bezplatně udělena licence na celé dílo v souladu s podmínkami této licence.\n"
"      d) Pokud se prostředek v upravené knihovně odkazuje na funkci nebo datovou tabulku, kterou má dodat aplikační program, jenž prostředek využívá, jinou než argument předávaný při volání prostředku, pak musíte vyvinout dobře míněnou snahu zajistit, aby daný prostředek fungoval i v případě, že aplikace takovou funkci nebo tabulku neposkytuje, a prováděl kteroukoli část svého poslání, jež zůstane smysluplná.\n\n" 

"(Kupříkladu posláním knihovní funkce pro výpočet druhé odmocniny je, aby byla naprosto jasně definovaná nezávisle na aplikaci. Pododdíl 2d tedy předepisuje, že jakákoli aplikací dodaná funkce nebo tabulka musí být volitelná: I když ji aplikace nedodá, funkce pro výpočet druhé odmocniny musí pořád počítat druhou odmocninu.)\n\n"

"Tyto požadavky se vztahují na upravené dílo jako celek. Pokud identifikovatelné oddíly tohoto díla nejsou odvozeny od Knihovny a lze je samy rozumně považovat za nezávislá a samostatná díla, pak když je šíříte jako samostatná díla, tato licence a její podmínky se na ně nevztahují. Ale pakliže stejné oddíly šíříte jako součást celku, kterým je dílo založené na Knihovně, musí šíření tohoto celku probíhat na základě podmínek této licence, a platnost toho, co povoluje jiným držitelům licence, se rozšiřuje na úplný celek, a tím na každou část bez ohledu na to, kdo ji napsal.\n\n"

"Účelem tohoto oddílu tedy není dovolávat se práv nebo zpochybňovat vaše práva na dílo, které jste napsali pouze vy; účelem je uplatňovat právo regulovat šíření odvozených nebo společných děl založených na Knihovně.\n\n"

"Mimoto, pouhé seskupení jiného díla nezaloženého na Knihovně spolu s Knihovnou (nebo dílem založeným na Knihovně) na jedno úložné či distribuční médium neznamená, že by se na ono jiné dílo pak vztahovala tato licence.\n\n"

"3. Můžete se rozhodnout použít pro danou kopii Knihovny místo této licence podmínky obyčejné GNU General Public License. Abyste to mohli udělat, musíte pozměnit všechna vyrozumění související s touto licencí tak, aby se odvolávala na obyčejnou GNU General Public License ve verzi 2, a ne na tuto licenci. (Pokud se objevila novější verze obyčejné GNU General Public License než 2, pak můžete namísto ní uvést tuto novější verzi, pokud chcete.) Žádné další změny v těchto vyrozuměních nedělejte.\n\n"

"Jakmile je tato změna v dané kopii provedena, je pro ni neodvolatelná, a tak pro všechny následné kopie a díla odvozená od této kopie platí obyčejná GNU General Public License.\n\n"

"Tato možnost je užitečná, jestliže si přejete zkopírovat část kódu Knihovny do programu, který není knihovnou.\n\n"

"4. Knihovnu (nebo její část či podle oddílu 2 dílo od ní odvozené) v objektovém kódu nebo ve spustitelné podobě smíte kopírovat a šířit v souladu s podmínkami z oddílů 1 a 2 výše za předpokladu, že k ní přiložíte úplný příslušný strojově čitelný zdrojový kód, který podle podmínek z oddílů 1 a 2 výše musí být šířen na médiu obvykle používaném pro výměnu softwaru.\n\n"

"Pokud se šíření objektového kódu provádí tak, že se nabídne přístup ke kopírování ze stanoveného místa, pak požadavku na šíření zdrojového kódu vyhovíte tím, že nabídnete obdobný přístup ke kopírování zdrojového kódu ze stejného místa, a to i přesto, že nenutíte třetí strany kopírovat zdrojový kód spolu s objektovým kódem.\n\n"

"5. Program, který neobsahuje žádnou odvozeninu od jakékoli části Knihovny, ale tím, že se s Knihovnou kompiluje nebo sestavuje, je určen k práci s ní, se nazývá \"dílo, které používá Knihovnu\". Takové dílo samo o sobě není dílem odvozeným od Knihovny, a proto nespadá do rozsahu platnosti této licence.\n\n"

"Avšak sestavením \"díla, které používá Knihovnu\" s Knihovnou vznikne spustitelný soubor, který je spíše odvozeninou od Knihovny (protože používá části Knihovny), než \"dílem, které používá Knihovnu\". Proto se tato licence tohoto spustitelného souboru týká. Podmínky pro šíření těchto spustitelných souborů stanovuje oddíl 6.\n\n"

"Pokud \"dílo, které používá Knihovnu\" používá obsah hlavičkového souboru, který je součástí knihovny, objektový kód pro toto dílo může být dílem odvozeným od knihovny, i když zdrojový kód jím není. Zda je tomu tak, je obzvláště významné, jestliže lze dílo sestavit i bez Knihovny nebo dílo samo je knihovna. Práh pro stanovení, zda tomu tak je, zákon přesně nedefinuje.\n\n"

"Používá-li takový objektový soubor jen numerické parametry, rozvržení a přístupové modifikátory datových struktur, a malá makra a malé inline funkce (o deseti a méně řádcích), pak se na použití tohoto objektového souboru nevztahuje žádné omezení bez ohledu na to, zda jde z právního hlediska o odvozené dílo. (Spustitelné soubory obsahující tento objektový kód plus části Knihovny budou stále spadat pod oddíl 6.)\n\n"

"Jinak, je-li dílo odvozeninou od Knihovny, smíte objektový kód šířit v souladu s podmínkami uvedenými v oddílu 6. Jakékoli spustitelné soubory obsahující toto dílo rovněž spadají pod oddíl 6, ať už jsou nebo nejsou sestaveny přímo s Knihovnou samou.\n\n"

"6. Výjimkou z výše uvedených oddílů je, že také smíte zkombinovat nebo sestavit \"dílo, které používá Knihovnu\" spolu s Knihovnou, čímž vznikne dílo obsahující části Knihovny, a šířit toto dílo za podmínek, které uznáte za vhodné, za předpokladu, že tyto podmínky zákazníkovi dovolují upravit si dílo pro vlastní použití a pro odlaďování takovýchto úprav provádět reverzní inženýrství.\n\n"

"Ke každé kopii výsledného díla musíte poskytnout dobře viditelné vyrozumění, že je v něm použita Knihovna a že na Knihovnu a na její používání se vztahuje tato licence. Musíte také přiložit kopii této licence. Jestliže dílo za běhu zobrazuje vyrozumění o autorských právech, musíte mezi ně zahrnout vyrozumění o autorských právech ke Knihovně a rovněž odkaz směrující uživatele ke kopii této licence. Musíte též udělat jednu z těchto věcí:\n\n"

"      a) Přiložte k dílu úplný příslušný strojově čitelný zdrojový kód Knihovny včetně jakýchkoli změn, které byly v díle použity (musí být šířeny v souladu s oddíly 1 a 2 výše); a je-li dílo spustitelný soubor sestavený s knihovnou, úplné strojově čitelné \"dílo, které používá knihovnu\" jako objektový kód a/nebo zdrojový kód, tak aby uživatel mohl Knihovnu upravit a potom znovu sestavit za účelem vytvoření upraveného spustitelného souboru obsahujícího upravenou Knihovnu. (Je jasné, že uživatel, který změní obsah souborů s definicemi v Knihovně nemusí být nutně schopen znovu zkompilovat aplikaci tak, aby používala modifikované definice.)\n"
"      b) Použijte pro sestavení s Knihovnou vhodný mechanizmus sdílení knihoven. Vhodný mechanizmus je ten, který (1) při běhu používá kopii knihovny, která v uživatelově počítačovém systému už je, místo toho, aby knihovní funkce kopíroval do spustitelného souboru, a (2) nainstaluje-li uživatel upravenou verzi knihovny, bude s ní fungovat správně, dokud tato upravená verze bude mít rozhraní kompatibilní s verzí, s níž bylo dílo vytvořeno.\n"
"      c) Přiložte k dílu psanou objednávku s platností nejméně na tři roky na poskytnutí materiálů uvedených v podsekci 6a výše stejnému uživateli, a to buď bezplatně, nebo za cenu nepřevyšující náklady na uskutečnění této dodávky.\n"
"      d) Šíříte-li dílo tak, že nabízíte přístup ke zkopírování ze stanoveného místa, nabídněte rovnocenný přístup ke zkopírování výše uvedených materiálů ze stejného místa.\n"
"      e) Ověřte, že uživatel kopii těchto materiálů už přijal, nebo že jste takovou kopii tomuto uživateli už poslali.\n\n" 

"7. Knihovní prostředky, které jsou dílem založeným na Knihovně, smíte umístit do jedné knihovny spolu s jinými knihovními prostředky, na které se tato licence nevztahuje, a takovouto kombinovanou knihovnu smíte šířit za předpokladu, že samostatné šíření původního díla založeného na Knihovně a stejně tak ostatních knihovních prostředků je umožněno jinak a že uděláte tyto dvě věci:\n\n"

"      a) Přiložíte ke kombinované knihovně kopii stejného díla založeného na Knihovně nezkombinovaného s žádnými jinými knihovními prostředky. Tuto kopii musíte šířit v souladu s podmínkami z oddílů uvedených výše.\n"
"      b) Poskytnete ke kombinované knihovně dobře viditelné vyrozumění o skutečnosti, že její část je dílem založeným na Knihovně, a vysvětlující, kde najít doplňující nezkombinovanou podobu stejného díla.\n\n" 

"8. Knihovnu nesmíte kopírovat, upravovat, udělovat na ni licenci, sestavovat nebo šířit jinak, než je výslovně stanoveno touto licencí. Jakákoli snaha kopírovat, upravovat, udělovat licenci na, sestavovat nebo šířit Knihovnu jinak je neplatná a automaticky vás zbavuje práv daných touto licencí. Nicméně stranám, které od vás obdržely kopie nebo práva podle této licence, nebudou jejich licence vypovězeny, dokud se jimi tyto strany budou v plném rozsahu řídit.\n\n"

"9. Protože jste tuto licenci nepodepsali, nežádá se po vás, abyste ji přijali. Avšak povolení upravovat nebo šířit Knihovnu nebo díla od ní odvozená nikde jinde nedostanete. Pokud tuto licenci nepřijmete, jsou takové činnosti zákonem zapovězeny. Proto úpravou nebo šířením Knihovny (nebo jakéhokoli díla založeného na Knihovně) dáváte najevo, že tuto licenci se všemi jejími podmínkami pro kopírování, šíření či úpravy Knihovny nebo děl na ní založených přijímáte, abyste tak mohli činit.\n\n"

"10. Pokaždé, když budete Knihovnu (nebo jakékoli dílo založené na Knihovně) šířit dále, příjemce automaticky od původního poskytovatele obdrží licenci na kopírování, šíření, sestavování nebo upravování Knihovny podléhající těmto podmínkám. Na uplatňování zde zaručených práv příjemci nesmíte uvalit žádná další omezení. Nejste odpovědní za prosazování, aby třetí strany tuto licenci dodržovali.\n\n"

"11. Jestliže jsou vám v důsledku soudního rozsudku nebo obvinění z nerespektování patentu či z jakéhokoli jiného důvodu (bez omezení na otázku patentů) uloženy podmínky (ať už soudním příkazem, dohodou nebo jinak), které jsou v rozporu s podmínkami této licence, od podmínek této licence vás neosvobozují. Nemůžete-li provádět šíření tak, abyste dostáli svým povinnostem daným touto licencí a současně jakýmkoli dalším relevantním povinnostem, nesmíte Knihovnu šířit vůbec. Pokud by například všem, kdo od vás přímo nebo nepřímo dostali kopii, neumožňovala nějaká patentová licence další šíření knihovny, aniž by museli zaplatit licenční poplatek, pak by jediným způsobem, jak neporušit ani patentovou ani tuto licenci, bylo se šíření Knihovny úplně zdržet.\n\n"

"Je-li za jakýchkoli zvláštních okolností kterákoli část tohoto oddílu neplatná nebo nevynutitelná, bude platit zbytek tohoto oddílu, a za jiných okolností bude platit tento oddíl celý.\n\n"

"Účelem tohoto oddílu není navádět vás k nerespektování jakýchkoli patentů nebo jiných nároků na vlastnická práva ani ke zpochybňování oprávněnosti jakýchkoli takovýchto nároků; jediným účelem tohoto oddílu je ochraňovat celistvost systému šíření svobodného softwaru, realizovaného praktikováním veřejných licencí. Mnoho lidí štědře přispělo k pestrému výběru softwaru, který je šířen prostřednictvím tohoto systému, s důvěrou v důsledné používání tohoto systému; rozhodnutí, zda je ochoten šířit software prostřednictvím jakéhokoli jiného systému, je na autorovi/dárci, a držitel licence nemůže nikomu vnucovat své rozhodnutí.\n\n"

"V tomto oddílu chceme důkladně vyjasnit, co je, jak se předpokládá, důsledkem zbytku této licence.\n\n"

"12. Pokud šíření a/nebo používání Knihovny v určitých zemích omezují buďto patenty, nebo skutečnost, že na použitá rozhraní existují autorská práva, smí původní držitel autorských práv, který pro Knihovnu použije tuto licenci, přidat výslovné omezení vylučující šíření v takových zemích, aby bylo šíření dovoleno pouze v rámci zemí, jež jím nebyly vyloučeny. V takovém případě se omezení zahrne do této licence, jako by bylo uvedeno v jejím textu.\n\n"

"13. Nadace Free Software Foundation může čas od času publikovat zrevidované a/nebo nové verze Lesser General Public License. Takovéto nové verze budou svým duchem podobné současné verzi, ale mohou se lišit v detailech, které se budou zaměřovat na nové problémy nebo záležitosti.\n\n"

"Každé verzi licence je dáno rozlišovací číslo verze. Jestliže Knihovna stanoví, že se na ni vztahuje konkrétní číslo verze této licence \"nebo jakákoli další verze\", můžete se řídit buď podmínkami uvedené verze, anebo podmínkami jakékoli další verze, kterou vydala nadace Free Software Foundation. Pokud Knihovna číslo verze licence neuvádí, můžete si vybrat jakoukoli verzi, kterou nadace Free Software Foundation kdy vydala.\n\n"

"14. Přejete-li si zahrnout části Knihovny do jiných svobodných programů, jejichž distribuční podmínky jsou se zde uváděnými podmínkami neslučitelné, napište dotyčnému autorovi a požádejte ho o svolení. Co se týče softwaru, na který má autorská práva nadace Free Software Foundation, pište na Free Software Foundation; někdy v takovém případě udělujeme výjimky. Naše rozhodnutí se řídí dvěma cíli: Uchovat svobodný statut všech odvozenin našeho svobodného softwaru a prosazovat sdílení a znovuvyužívání softwaru všeobecně.\n\n"

"ZÁRUKA SE NEPOSKYTUJE\n\n"

"15. PROTOŽE SE NA KNIHOVNU UDĚLUJE BEZPLATNÁ LICENCE, NEVZTAHUJE SE NA NI V MÍŘE TOLEROVANÉ PŘÍSLUŠNÝMI ZÁKONY ŽÁDNÁ ZÁRUKA. POKUD NEBYLO PÍSEMNĚ STANOVENO JINAK, DRŽITELÉ AUTORSKÝCH PRÁV A/NEBO JINÉ STRANY POSKYTUJÍ KNIHOVNU \"TAK, JAK JE\" BEZ JAKÉHOKOLI DRUHU ZÁRUKY, AŤ VÝSLOVNÉ NEBO PŘEDPOKLÁDANÉ, VČETNĚ PŘEDPOKLÁDANÝCH ZÁRUK OBCHODOVATELNOSTI A VHODNOSTI PRO URČITÝ ÚČEL. VEŠKERÁ RIZIKA, CO SE KVALITY A FUNGOVÁNÍ KNIHOVNY TÝČE, NA SEBE BERETE VY. PAKLIŽE BY SE UKÁZALO, ŽE KNIHOVNA JE VADNÁ, UHRADÍTE NÁKLADY NA VEŠKEROU POTŘEBNOU ÚDRŽBU A OPRAVY.\n\n"

"16. ŽÁDNÝ DRŽITEL AUTORSKÝCH PRÁV ANI ŽÁDNÁ JINÁ STRANA, KTERÁ SMÍ KNIHOVNU UPRAVOVAT A/NEBO ŠÍŘIT DÁLE, JAK JE POVOLENO VÝŠE, SE VÁM V ŽÁDNÉM PŘÍPADĚ, NENÍ-LI TO VYŽADOVÁNO PŘÍSLUŠNÝMI ZÁKONY NEBO PÍSEMNĚ ODSOUHLASENO, NEBUDE ZODPOVÍDAT ZA ŠKODY, VČETNĚ JAKÝCHKOLI OBECNÝCH, MIMOŘÁDNÝCH, NÁHODNÝCH NEBO NÁSLEDNÝCH ŠKOD VYPLÝVAJÍCÍCH Z POUŽITÍ NEBO NEMOŽNOSTI POUŽITÍ KNIHOVNY (VČETNĚ ZTRÁTY ČI POKAŽENÍ DAT, ZTRÁT UTRPĚNÝCH VÁMI ČI TŘETÍMI STRANAMI NEBO NESCHOPNOSTI KNIHOVNY FUNGOVAT S JAKÝMKOLI JINÝM SOFTWAREM), A TO I V PŘÍPADĚ, ŽE TENTO DRŽITEL NEBO TŘETÍ STRANA BYLY NA MOŽNOST TAKOVÝCH ŠKOD UPOZORNĚNI.\n\n";

extern wxLocale locale2;  

void AboutOpenDlg::CreateControls()
{    
   // create the hyperlink:
    wxHyperlinkCtrl * mURL = new wxHyperlinkCtrl;
    mURL->Create(this, CD_ABOUT_URL, wxEmptyString, _("http://www.software602.com"), wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxHL_CONTEXTMENU|wxHL_ALIGN_LEFT );

////@begin AboutOpenDlg content construction
    AboutOpenDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxGROW|wxALL, 0);

    wxBitmap mBitmapBitmap(wxNullBitmap);
    mBitmap = new wxStaticBitmap;
    mBitmap->Create( itemDialog1, CD_ABOUT_BITMAP, mBitmapBitmap, wxDefaultPosition, wxSize(111, 114), 0 );
    itemBoxSizer3->Add(mBitmap, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mSizer = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer3->Add(mSizer, 1, wxGROW|wxALL, 0);

    mMainName = new wxStaticText;
    mMainName->Create( itemDialog1, wxID_STATIC, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    mMainName->SetFont(wxFont(12, wxSWISS, wxNORMAL, wxNORMAL, FALSE, _T("MS Sans Serif")));
    mSizer->Add(mMainName, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    mBuild = new wxStaticText;
    mBuild->Create( itemDialog1, CD_ABOUT_BUILD, _("Build:"), wxDefaultPosition, wxDefaultSize, 0 );
    mSizer->Add(mBuild, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    mSizer->Add(5, 5, 1, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    mCopyr1 = new wxStaticText;
    mCopyr1->Create( itemDialog1, wxID_STATIC, _("Authors: Janus Drozd, Vitezslav Boukal, Vaclav Pecuch and others"), wxDefaultPosition, wxDefaultSize, 0 );
    mSizer->Add(mCopyr1, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    mCopyr2 = new wxStaticText;
    mCopyr2->Create( itemDialog1, wxID_STATIC, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    mSizer->Add(mCopyr2, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    mURL = (wxHyperlinkCtrl*) FindWindow(CD_ABOUT_URL);
    wxASSERT( mURL != NULL );
    mSizer->Add(mURL, 0, wxALIGN_LEFT|wxALL, 5);

    mLicenc = new wxTextCtrl;
    mLicenc->Create( itemDialog1, CD_ABOUT_LICENC, _T(""), wxDefaultPosition, wxSize(-1, 180), wxTE_MULTILINE|wxTE_READONLY );
    itemBoxSizer2->Add(mLicenc, 1, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxButton* itemButton13 = new wxButton;
    itemButton13->Create( itemDialog1, wxID_CANCEL, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton13->SetDefault();
    itemBoxSizer2->Add(itemButton13, 0, wxALIGN_RIGHT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

////@end AboutOpenDlg content construction
    wxBitmap bmp(logo_xpm);
    mBitmap->SetBitmap(bmp);
    wxString vers_info;
#if VERS_2==0
    vers_info.Printf(wxT(" %d"), VERS_1);
#else
    vers_info.Printf(wxT(" %d.%d"), VERS_1, VERS_2);
#endif
    mMainName->SetLabel(wxString(_("602SQL Open Server")) + vers_info);

    vers_info.Printf(_("Version %u.%u, build %u"), VERS_1, VERS_2, VERS_4);
    mBuild->SetLabel(vers_info);

    wxString locname = locale2.GetCanonicalName();
    mLicenc->SetValue(locname==wxT("cs") || locname==wxT("cs_CZ") ? wxString(lic_cs, wxConvUTF8) :
                      wxString(lic_en, *wxConvCurrent));  
}

/*!
 * Should we show tooltips?
 */

bool AboutOpenDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap AboutOpenDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin AboutOpenDlg bitmap retrieval
    return wxNullBitmap;
////@end AboutOpenDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon AboutOpenDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin AboutOpenDlg icon retrieval
    return wxNullIcon;
////@end AboutOpenDlg icon retrieval
}
