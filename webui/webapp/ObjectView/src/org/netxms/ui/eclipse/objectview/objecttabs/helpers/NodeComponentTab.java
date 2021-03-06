/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Victor Kirhenshtein
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.ui.eclipse.objectview.objecttabs.helpers;

import org.eclipse.core.commands.Command;
import org.eclipse.core.commands.State;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.commands.ICommandService;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Common node component tab class
 */
public abstract class NodeComponentTab extends ObjectTab
{
   public static final int COLUMN_ID = 0;
   public static final int COLUMN_NAME = 1;
   public static final int COLUMN_STATUS = 2;
   public static final int COLUMN_PEER_GATEWAY = 3;
   public static final int COLUMN_LOCAL_SUBNETS = 4;
   public static final int COLUMN_REMOTE_SUBNETS = 5;

   protected SortableTableViewer viewer;
   protected SessionListener sessionListener = null;
   protected Action actionExportToCsv;

   protected boolean showFilter = true;
   protected FilterText filterText;
   protected NodeComponentTabFilter filter;
   protected Composite mainArea;
   private NXCSession session;
   private boolean objectsFullySync;

   /*
    * (non-Javadoc)
    * 
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createTabContent(Composite parent)
   {
      session = ConsoleSharedData.getSession();
      final IDialogSettings settings = Activator.getDefault().getDialogSettings();
      showFilter = safeCast(settings.get(getFilterSettingName()), settings.getBoolean(getFilterSettingName()), showFilter); // $NON-NLS-1$

      IDialogSettings coreSettings = ConsoleSharedData.getSettings();
      objectsFullySync = coreSettings.getBoolean("ObjectsFullSync");    
      
      // Create VPN area
      mainArea = new Composite(parent, SWT.BORDER);
      FormLayout formLayout = new FormLayout();
      mainArea.setLayout(formLayout);

      // Create filter
      filterText = new FilterText(mainArea, SWT.NONE);
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });
      filterText.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            settings.put(getFilterSettingName(), showFilter); // $NON-NLS-1$
         }
      });

      Action action = new Action() {
         @Override
         public void run()
         {
            enableFilter(false);
            ICommandService service = (ICommandService)PlatformUI.getWorkbench().getService(ICommandService.class);
            Command command = service.getCommand("org.netxms.ui.eclipse.objectview.commands.show_filter"); //$NON-NLS-1$
            State state = command.getState("org.netxms.ui.eclipse.objectview.commands.show_filter.state"); //$NON-NLS-1$
            state.setValue(false);
            service.refreshElements(command.getId(), null);
         }
      };
      setFilterCloseAction(action);

      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterText.setLayoutData(fd);

      createViewer();
      createActions();
      createPopupMenu();

      // Set initial focus to filter input line
      if (showFilter)
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly

      sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() == SessionNotification.OBJECT_CHANGED)
            {
               AbstractObject object = (AbstractObject)n.getObject();
               if ((object != null) && needRefreshOnObjectChange(object) && (getObject() != null)
                     && object.isDirectChildOf(getObject().getObjectId()))
               {
                  viewer.getControl().getDisplay().asyncExec(new Runnable() {
                     @Override
                     public void run()
                     {
                        refresh();
                     }
                  });
               }
            }
         }
      };
      ConsoleSharedData.getSession().addListener(sessionListener);
   }

   /**
    * Create filter control
    * 
    * @return filter control
    */
   protected FilterText createFilterText()
   {
      return new FilterText(mainArea, SWT.NONE);
   }

   /**
    * Returns created viewer
    * 
    * @return viewer
    */
   protected abstract void createViewer();

   /**
    * Return filter setting name
    * 
    * @return filter setting name
    */
   public abstract String getFilterSettingName();

   /**
    * Returns if view should be updated depending on provided object
    * 
    * @param object updated object
    * @return if tab should be refreshed
    */
   public abstract boolean needRefreshOnObjectChange(AbstractObject object);

   /*
    * (non-Javadoc)
    * 
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#dispose()
    */
   @Override
   public void dispose()
   {
      ConsoleSharedData.getSession().removeListener(sessionListener);
      super.dispose();
   }

   /**
    * @param b
    * @param defval
    * @return
    */
   protected static boolean safeCast(String s, boolean b, boolean defval)
   {
      return (s != null) ? b : defval;
   }

   /**
    * @param action
    */
   private void setFilterCloseAction(Action action)
   {
      filterText.setCloseAction(action);
   }

   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   public void enableFilter(boolean enable)
   {
      showFilter = enable;
      filterText.setVisible(showFilter);
      FormData fd = (FormData)viewer.getTable().getLayoutData();
      fd.top = enable ? new FormAttachment(filterText, 0, SWT.BOTTOM) : new FormAttachment(0, 0);
      mainArea.layout();
      if (enable)
      {
         filterText.setFocus();
      }
      else
      {
         filterText.setText(""); //$NON-NLS-1$
         onFilterModify();
      }
   }

   /**
    * Handler for filter modification
    */
   public void onFilterModify()
   {
      if (filter == null)
         return;

      final String text = filterText.getText();
      filter.setFilterString(text);
      viewer.refresh(false);
   }

   /**
    * Create actions
    */
   protected void createActions()
   {
      actionExportToCsv = new ExportToCsvAction(getViewPart(), viewer, true);
   }

   /**
    * Create pop-up menu
    */
   protected void createPopupMenu()
   {
      // Create menu manager.
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager manager)
         {
            fillContextMenu(manager);
         }
      });

      // Create menu.
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);

      // Register menu for extension.
      if (getViewPart() != null)
         getViewPart().getSite().registerContextMenu(menuMgr, viewer);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected abstract void fillContextMenu(IMenuManager manager);

   /*
    * (non-Javadoc)
    * 
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#currentObjectUpdated(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public void currentObjectUpdated(AbstractObject object)
   {
      objectChanged(object);
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#showForObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean showForObject(AbstractObject object)
   {
      return (object instanceof AbstractNode);
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#getSelectionProvider()
    */
   @Override
   public ISelectionProvider getSelectionProvider()
   {
      return viewer;
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#selected()
    */
   @Override
   public void selected()
   {
      // Check/uncheck menu items
      ICommandService service = (ICommandService)PlatformUI.getWorkbench().getService(ICommandService.class);

      Command command = service.getCommand("org.netxms.ui.eclipse.objectview.commands.show_filter"); //$NON-NLS-1$
      State state = command.getState("org.netxms.ui.eclipse.objectview.commands.show_filter.state"); //$NON-NLS-1$
      state.setValue(showFilter);
      service.refreshElements(command.getId(), null);
      
      checkAndSyncChildren(getObject());
      super.selected();
      refresh();
   }

   @Override
   public void objectChanged(AbstractObject object)
   {
      checkAndSyncChildren(object);
      refresh();
   }
   
   /**
    * Check is child objects are synchronized and synchronize if needed
    * 
    * @param object current object
    */
   private void checkAndSyncChildren(AbstractObject object)
   {
      if (!objectsFullySync && isActive())
      {
         if (object instanceof Node && object.hasChildren() && !session.areChildrenSynchronized(object.getObjectId()))
         {
            syncChildren(object);
         }
      }      
   }

   /**
    * Sync object children form server
    * 
    * @param object current object
    */
   private void syncChildren(AbstractObject object)
   {
      final Composite label = new Composite(mainArea, SWT.NONE);
      label.setLayout(new GridLayout());
      label.setBackground(label.getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));
      
      Label labelText = new Label(label, SWT.CENTER);
      labelText.setText("Loading...");
      labelText.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, true, true));      
      labelText.setBackground(label.getBackground());
      
      label.moveAbove(null);
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top =  new FormAttachment(0, 0);
      fd.bottom = new FormAttachment(100, 0);
      fd.right = new FormAttachment(100, 0);
      label.setLayoutData(fd);
      mainArea.layout();
      
      ConsoleJob job = new ConsoleJob("Synchronize node components", null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.syncChildren(object);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  refresh();
                  label.dispose();
                  mainArea.layout();                  
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot synchronize node components";
         }
      };
      job.setUser(false);
      job.start();
   }
}
